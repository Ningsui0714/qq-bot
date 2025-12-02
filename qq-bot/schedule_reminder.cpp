#include "schedule_reminder.h"
#include "utils.h"
#include "schedule_loader.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <ctime>

// 与 schedule_set.cpp 保持一致的持久化文件路径
static const char* SCHEDULE_FILE = "persistent_schedules.json";

std::tm ScheduleReminder::term_start_date = []() {
    std::tm tm{};
    // 默认值：2024年9月2日（可通过命令修改）
    tm.tm_year = 2024 - 1900;
    tm.tm_mon = 8; // 9月（0-based）
    tm.tm_mday = 2;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    tm.tm_isdst = -1; // 让库自行判断夏令时
    // 规范化为有效的时间结构（确保 tm_wday 可用）
    std::mktime(&tm);
    return tm;
}();

ScheduleType ScheduleReminder::get_schedule_type(const std::tm& date) {
    // 5月1日至10月1日为夏季作息
    if ((date.tm_mon > 4 && date.tm_mon < 9) ||  // 6-8月
        (date.tm_mon == 4 && date.tm_mday >= 1) ||  // 5月1日后
        (date.tm_mon == 9 && date.tm_mday == 1)) {  // 10月1日当天
        return ScheduleType::SUMMER;
    }
    return ScheduleType::WINTER;
}

int ScheduleReminder::get_week_of_term(const std::tm& date) {
    // 将两个 tm 规范化到 time_t（本地时区）
    std::tm start = term_start_date;
    std::tm target = date;
    // 清零时间部分，确保按日期计算
    start.tm_hour = start.tm_min = start.tm_sec = 0;
    target.tm_hour = target.tm_min = target.tm_sec = 0;
    start.tm_isdst = -1;
    target.tm_isdst = -1;

    std::time_t t_start = std::mktime(&start);
    std::time_t t_target = std::mktime(&target);

    // 计算天数差
    double seconds = std::difftime(t_target, t_start);
    // 向下取整天数（负数时也能工作）
    long long days = static_cast<long long>(seconds / 86400.0);

    // 学期第几周：从 1 开始，0-6 天为第 1 周，7-13 为第 2 周...
    int week = static_cast<int>(days / 7) + 1;
    if (week < 1) {
        week = 1; // 学期开始前，按第 1 周处理
    }
    return week;
}

bool ScheduleReminder::is_course_on_date(const Schedule& course, const std::tm& date) {
    int current_week = get_week_of_term(date);
    // 检查周数是否在课程范围内
    if (current_week < course.get_start_week() || current_week > course.get_end_week()) {
        return false;
    }
    // 检查星期是否匹配（注意：tm_wday周日为0，需要转换）
    int course_weekday = course.get_weekday(); // 1-7（周一到周日）
    int date_weekday = date.tm_wday == 0 ? 7 : date.tm_wday; // 转换为1-7
    return course_weekday == date_weekday;
}

bool ScheduleReminder::set_term_start_date(const std::string& date_str) {
    // 规范化 YYYY-M-D 到 YYYY-MM-DD
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
    write_log("学期开始日期已设置为: " + norm);
    return true;
}

std::vector<Schedule> ScheduleReminder::get_courses_on_date(
    const std::string& qq_number,
    const std::tm& target_date
) {
    // 从持久化文件中获取全部课表，再筛选当前用户
    auto all_schedules = ScheduleLoader::load_from_file(SCHEDULE_FILE);
    auto it = all_schedules.find(qq_number);
    if (it == all_schedules.end()) {
        return {};
    }

    // 筛选当天有课的课程
    std::vector<Schedule> result;
    result.reserve(it->second.size());
    for (const auto& course : it->second) {
        if (is_course_on_date(course, target_date)) {
            result.push_back(course);
        }
    }

    // 按上课开始节次排序
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
    // 规范化
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

        // 根据作息类型显示具体时间
        if (course.get_start_class() <= 4) {
            // 上午课程（冬夏相同）
            ss << (course.get_start_class() <= 2 ? u8"8:00-9:40" : u8"10:10-11:50");
        }
        else {
            // 下午和晚上课程（冬夏不同）
            if (type == ScheduleType::WINTER) {
                switch (course.get_start_class()) {
                case 5: ss << u8"14:00-15:35"; break;
                case 7: ss << u8"15:55-17:30"; break;
                case 9: ss << u8"18:30-20:05"; break;
                case 11: ss << u8"20:15-21:50"; break;
                default: break;
                }
            }
            else {
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
    std::tm tomorrow = *std::localtime(&now);
#endif
    tomorrow.tm_mday += 1;
    tomorrow.tm_hour = 0;
    tomorrow.tm_min = 0;
    tomorrow.tm_sec = 0;
    tomorrow.tm_isdst = -1;
    std::mktime(&tomorrow); // 处理月份进位与规范化

    auto courses = get_courses_on_date(qq_number, tomorrow);
    if (courses.empty()) {
        return u8"明天没有课程哦～";
    }

    std::stringstream ss;
    ss << u8"📢 明日课程提醒：\n";
    ScheduleType type = get_schedule_type(tomorrow);

    // 内容格式同今日课程
    for (const auto& course : courses) {
        ss << course.get_name() << u8"（周" << course.get_weekday() << u8"）";
        ss << u8" 第" << course.get_start_class() << u8"-" << course.get_end_class() << u8"节 ";

        if (course.get_start_class() <= 4) {
            ss << (course.get_start_class() <= 2 ? u8"8:00-9:40" : u8"10:10-11:50");
        }
        else {
            if (type == ScheduleType::WINTER) {
                switch (course.get_start_class()) {
                case 5: ss << u8"14:00-15:35"; break;
                case 7: ss << u8"15:55-17:30"; break;
                case 9: ss << u8"18:30-20:05"; break;
                case 11: ss << u8"20:15-21:50"; break;
                default: break;
                }
            }
            else {
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