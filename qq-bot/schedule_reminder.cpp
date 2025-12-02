#include "schedule_reminder.h"
#include "utils.h"
#include "schedule_loader.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <fstream>

// 与 schedule_set.cpp 保持一致的持久化课表文件路径（仅用于读取课程，不含学期开始日期）
static const char* SCHEDULE_FILE = "persistent_schedules.json";
// 学期开始日期单独持久化文件（简单文本：YYYY-MM-DD）
static const char* TERM_START_FILE = "term_start_date.txt";

std::tm ScheduleReminder::term_start_date = []() {
    // 默认值：2024-09-02
    auto fallback = []() {
        std::tm tm{};
        tm.tm_year = 2024 - 1900;
        tm.tm_mon = 8; // 9月
        tm.tm_mday = 2;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        tm.tm_isdst = -1;
        std::mktime(&tm);
        return tm;
    };

    std::ifstream ifs(TERM_START_FILE, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
        write_log(u8"学期开始日期持久化文件不存在，使用默认值 2024-09-02");
        return fallback();
    }

    std::string content;
    std::getline(ifs, content);
    content = trim_space(content);
    if (content.empty()) {
        write_log(u8"学期开始日期文件为空，使用默认值 2024-09-02");
        return fallback();
    }

    std::tm tm{};
    std::istringstream ss(content);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail()) {
        write_log(u8"学期开始日期文件解析失败，内容：" + content + "，使用默认值 2024-09-02");
        return fallback();
    }

    tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0; tm.tm_isdst = -1;
    std::mktime(&tm);
    write_log(u8"已从持久化文件加载学期开始日期: " + content);
    return tm;
}();

ScheduleType ScheduleReminder::get_schedule_type(const std::tm& date) {
    if ((date.tm_mon > 4 && date.tm_mon < 9) ||
        (date.tm_mon == 4 && date.tm_mday >= 1) ||
        (date.tm_mon == 9 && date.tm_mday == 1)) {
        return ScheduleType::SUMMER;
    }
    return ScheduleType::WINTER;
}

int ScheduleReminder::get_week_of_term(const std::tm& date) {
    std::tm start = term_start_date;
    std::tm target = date;
    start.tm_hour = start.tm_min = start.tm_sec = 0;
    target.tm_hour = target.tm_min = target.tm_sec = 0;
    start.tm_isdst = -1;
    target.tm_isdst = -1;

    std::time_t t_start = std::mktime(&start);
    std::time_t t_target = std::mktime(&target);

    double seconds = std::difftime(t_target, t_start);
    long long days = static_cast<long long>(seconds / 86400.0);

    int week = static_cast<int>(days / 7) + 1;
    if (week < 1) {
        week = 1;
    }
    return week;
}

bool ScheduleReminder::is_course_on_date(const Schedule& course, const std::tm& date) {
    int current_week = get_week_of_term(date);
    if (current_week < course.get_start_week() || current_week > course.get_end_week()) {
        return false;
    }
    int course_weekday = course.get_weekday();
    int date_weekday = date.tm_wday == 0 ? 7 : date.tm_wday;
    return course_weekday == date_weekday;
}

bool ScheduleReminder::set_term_start_date(const std::string& date_str) {
    auto normalize = [](std::string s) {
        std::istringstream iss(s);
        std::string y, m, d;
        if (std::getline(iss, y, '-') && std::getline(iss, m, '-') && std::getline(iss, d)) {
            auto trim = [](std::string v) {
                v = trim_space(v);
                return v;
            };
            auto pad2 = [](const std::string& v) {
                return v.size() == 1 ? std::string("0") + v : v;
            };
            y = trim(y); m = pad2(trim(m)); d = pad2(trim(d));
            return y + "-" + m + "-" + d;
        }
        return s;
    };

    std::string norm = normalize(date_str);
    std::istringstream ss(norm);
    std::tm tm{};
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail()) return false;

    tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0; tm.tm_isdst = -1;
    std::mktime(&tm);
    term_start_date = tm;

    // 持久化覆盖写入
    std::ofstream ofs(TERM_START_FILE, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!ofs.is_open()) {
        write_log("学期开始日期写入失败（无法打开文件）: " + std::string(TERM_START_FILE));
        return false;
    }
    ofs.write(norm.c_str(), static_cast<std::streamsize>(norm.size()));
    ofs.flush();
    if (!ofs.good()) {
        write_log("学期开始日期写入失败（写入错误）: " + std::string(TERM_START_FILE));
        return false;
    }

    write_log("学期开始日期已设置并持久化为: " + norm);
    return true;
}

std::vector<Schedule> ScheduleReminder::get_courses_on_date(
    const std::string& qq_number,
    const std::tm& target_date
) {
    auto all_schedules = ScheduleLoader::load_from_file(SCHEDULE_FILE);
    auto it = all_schedules.find(qq_number);
    if (it == all_schedules.end()) {
        return {};
    }

    std::vector<Schedule> result;
    result.reserve(it->second.size());
    for (const auto& course : it->second) {
        if (is_course_on_date(course, target_date)) {
            result.push_back(course);
        }
    }

    std::sort(result.begin(), result.end(), [](const Schedule& a, const Schedule& b) {
        if (a.get_start_class() != b.get_start_class()) return a.get_start_class() < b.get_start_class();
        if (a.get_end_class() != b.get_end_class()) return a.get_end_class() < b.get_end_class();
        return a.get_name() < b.get_name();
    });
    return result;
}

std::string ScheduleReminder::get_today_courses_reminder(const std::string& qq_number) {
    std::time_t now = std::time(nullptr);
    std::tm today{};
#if defined(_MSC_VER)
    localtime_s(&today, &now);
#else
    today = *std::localtime(&now);
#endif
    today.tm_isdst = -1;
    std::mktime(&today);

    auto courses = get_courses_on_date(qq_number, today);
    if (courses.empty()) {
        return u8"今天没有课程哦～";
    }

    std::stringstream ss;
    ss << u8"今日课程安排：\n";
    ScheduleType type = get_schedule_type(today);

    for (const auto& course : courses) {
        ss << course.get_name() << u8"（周" << course.get_weekday() << u8"）";
        ss << u8" 第" << course.get_start_class() << u8"-" << course.get_end_class() << u8"节 ";
        if (course.get_start_class() <= 4) {
            ss << (course.get_start_class() <= 2 ? u8"8:00-9:40" : u8"10:10-11:50");
        } else {
            if (type == ScheduleType::WINTER) {
                switch (course.get_start_class()) {
                case 5: ss << u8"14:00-15:35"; break;
                case 7: ss << u8"15:55-17:30"; break;
                case 9: ss << u8"18:30-20:05"; break;
                case 11: ss << u8"20:15-21:50"; break;
                default: break;
                }
            } else {
                switch (course.get_start_class()) {
                case 5: ss << u8"14:30-16:05"; break;
                case 7: ss << u8"16:25-17:50"; break;
                case 9: ss << u8"19:00-20:35"; break;
                case 11: ss << u8"20:45-22:20"; break;
                default: break;
                }
            }
        }
        ss << "\n";
    }
    return ss.str();
}

std::string ScheduleReminder::get_tomorrow_courses_reminder(const std::string& qq_number) {
    std::time_t now = std::time(nullptr);
    std::tm tomorrow{};
#if defined(_MSC_VER)
    localtime_s(&tomorrow, &now);
#else
    std::tm tomorrow_local = *std::localtime(&now);
    tomorrow = tomorrow_local;
#endif
    tomorrow.tm_mday += 1;
    tomorrow.tm_hour = 0;
    tomorrow.tm_min = 0;
    tomorrow.tm_sec = 0;
    tomorrow.tm_isdst = -1;
    std::mktime(&tomorrow);

    auto courses = get_courses_on_date(qq_number, tomorrow);
    if (courses.empty()) {
        return u8"明天没有课程哦～";
    }

    std::stringstream ss;
    ss << u8"📢 明日课程提醒：\n";
    ScheduleType type = get_schedule_type(tomorrow);

    for (const auto& course : courses) {
        ss << course.get_name() << u8"（周" << course.get_weekday() << u8"）";
        ss << u8" 第" << course.get_start_class() << u8"-" << course.get_end_class() << u8"节 ";
        if (course.get_start_class() <= 4) {
            ss << (course.get_start_class() <= 2 ? u8"8:00-9:40" : u8"10:10-11:50");
        } else {
            if (type == ScheduleType::WINTER) {
                switch (course.get_start_class()) {
                case 5: ss << u8"14:00-15:35"; break;
                case 7: ss << u8"15:55-17:30"; break;
                case 9: ss << u8"18:30-20:05"; break;
                case 11: ss << u8"20:15-21:50"; break;
                default: break;
                }
            } else {
                switch (course.get_start_class()) {
                case 5: ss << u8"14:30-16:05"; break;
                case 7: ss << u8"16:25-17:50"; break;
                case 9: ss << u8"19:00-20:35"; break;
                case 11: ss << u8"20:45-22:20"; break;
                default: break;
                }
            }
        }
        ss << "\n";
    }
    return ss.str();
}