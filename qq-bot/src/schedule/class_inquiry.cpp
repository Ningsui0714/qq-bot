#include "reply_generator.h"
#include "config.h"
#include "utils.h"
#include "schedule.h"
#include "schedule_loader.h"
#include "schedule_reminder.h"
#include "group_mapping.h"
#include "member_cache.h"
#include "onebot_ws_api.h"
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <ctime>
#include <sstream>
#include <iomanip>

// 全局常量定义
static const char* SCHEDULE_FILE = "persistent_schedules.json";

// 获取当前时间对应的节次（按冬季/夏季作息自动适配）
static int get_current_class_period() {
    std::time_t now = std::time(nullptr);
    std::tm current_time{};
#if defined(_MSC_VER)
    localtime_s(&current_time, &now);
#else
    current_time = *std::localtime(&now);
#endif
    current_time.tm_isdst = -1;
    std::mktime(&current_time);

    int hour = current_time.tm_hour;
    int minute = current_time.tm_min;
    ScheduleType schedule_type = ScheduleReminder::get_schedule_type(current_time);

    // 按作息类型判断当前节次（只判断核心上课时间段）
    if (schedule_type == ScheduleType::WINTER) {
        // 冬季作息：上午8:00开始，下午14:00开始
        if (hour >= 8 && hour < 10) return (hour == 8 && minute >= 0) ? 1 : 2;
        if (hour >= 10 && hour < 12) return (hour == 10 && minute >= 10) ? 3 : 4;
        if (hour >= 14 && hour < 16) return (hour == 14 && minute >= 0) ? 5 : 6;
        if (hour >= 16 && hour < 18) return (hour == 16 && minute >= 55) ? 7 : 8;
        if (hour >= 18 && hour < 20) return (hour == 18 && minute >= 30) ? 9 : 10;
        if (hour >= 20 && hour < 22) return (hour == 20 && minute >= 15) ? 11 : 12;
    }
    else {
        // 夏季作息：上午8:00开始，下午14:30开始
        if (hour >= 8 && hour < 10) return (hour == 8 && minute >= 0) ? 1 : 2;
        if (hour >= 10 && hour < 12) return (hour == 10 && minute >= 10) ? 3 : 4;
        if (hour >= 14 && hour < 16) return (hour == 14 && minute >= 30) ? 5 : 6;
        if (hour >= 16 && hour < 18) return (hour == 16 && minute >= 25) ? 7 : 8;
        if (hour >= 19 && hour < 21) return (hour == 19 && minute >= 0) ? 9 : 10;
        if (hour >= 21 && hour < 23) return (hour == 21 && minute >= 45) ? 11 : 12;
    }
    return -1; // 非上课时间
}

// 获取用户下一节有课的时间信息（原字符串版，仍保留给其他调用处）
static std::string get_next_class_info(const std::string& qq_number) {
    std::time_t now = std::time(nullptr);
    std::tm current_time{};
#if defined(_MSC_VER)
    localtime_s(&current_time, &now);
#else
    current_time = *std::localtime(&now);
#endif
    current_time.tm_isdst = -1;
    std::mktime(&current_time);

    // 遍历未来7天（包括今天）查找最近的课程
    for (int day_offset = 0; day_offset < 7; ++day_offset) {
        std::tm target_date = current_time;
        target_date.tm_mday += day_offset;
        std::mktime(&target_date); // 自动处理月份进位

        std::vector<Schedule> courses = ScheduleReminder::get_courses_on_date(qq_number, target_date);
        if (courses.empty()) continue;

        // 筛选当前时间之后的课程（当天）或所有课程（未来天数）
        std::vector<Schedule> valid_courses;
        int current_period = (day_offset == 0) ? get_current_class_period() : -2;

        for (const auto& course : courses) {
            if (day_offset == 0 && course.get_start_class() <= current_period) {
                continue; // 跳过今天已上过的课
            }
            valid_courses.push_back(course);
        }

        if (!valid_courses.empty()) {
            // 按节次排序，取最早的课程
            std::sort(valid_courses.begin(), valid_courses.end(), [](const Schedule& a, const Schedule& b) {
                return a.get_start_class() < b.get_start_class();
                });
            const Schedule& next_course = valid_courses[0];

            // 构造日期字符串
            char date_buf[32];
            std::strftime(date_buf, sizeof(date_buf), "%m-%d", &target_date);
            std::string week_str = u8"周" + std::to_string(target_date.tm_wday == 0 ? 7 : target_date.tm_wday);

            // 构造上课时间字符串
            ScheduleType type = ScheduleReminder::get_schedule_type(target_date);
            std::string time_str;
            if (next_course.get_start_class() <= 4) {
                time_str = (next_course.get_start_class() <= 2) ? u8"8:00-9:40" : u8"10:10-11:50";
            }
            else {
                if (type == ScheduleType::WINTER) {
                    switch (next_course.get_start_class()) {
                    case 5: time_str = u8"14:00-15:35"; break;
                    case 7: time_str = u8"15:55-17:30"; break;
                    case 9: time_str = u8"18:30-20:05"; break;
                    case 11: time_str = u8"20:15-21:50"; break;
                    default: time_str = u8"未知时段";
                    }
                }
                else {
                    switch (next_course.get_start_class()) {
                    case 5: time_str = u8"14:30-16:05"; break;
                    case 7: time_str = u8"16:25-17:50"; break;
                    case 9: time_str = u8"19:00-20:35"; break;
                    case 11: time_str = u8"20:45-22:20"; break;
                    default: time_str = u8"未知时段";
                    }
                }
            }

            return std::string(u8"下一节：") + next_course.get_name() + u8"（" + date_buf + week_str + u8"）" +
                u8" 第" + std::to_string(next_course.get_start_class()) + u8"-" + std::to_string(next_course.get_end_class()) +
                u8"节 " + time_str;
        }
    }

    return u8"未来7天暂无课程安排～";
}

// 计算某门课的下课时间（小时:分钟）
static void get_course_end_clock(ScheduleType type, int end_class, int& end_h, int& end_m)
{
    if (end_class <= 2) { end_h = 9;  end_m = 40; return; }
    if (end_class <= 4) { end_h = 11; end_m = 50; return; }

    if (type == ScheduleType::WINTER) {
        if (end_class <= 6) { end_h = 15; end_m = 35; return; }
        if (end_class <= 8) { end_h = 17; end_m = 30; return; }
        if (end_class <= 10){ end_h = 20; end_m = 5;  return; }
        /* 11-12 */          end_h = 21; end_m = 50; return;
    } else {
        if (end_class <= 6) { end_h = 16; end_m = 5;  return; }
        if (end_class <= 8) { end_h = 17; end_m = 50; return; }
        if (end_class <= 10){ end_h = 20; end_m = 35; return; }
        /* 11-12 */          end_h = 22; end_m = 20; return;
    }
}

// 新增：计算某门课的上课时间（小时:分钟）
static void get_course_start_clock(ScheduleType type, int start_class, int& sh, int& sm)
{
    if (start_class <= 2) { sh = 8;  sm = 0;   return; }
    if (start_class <= 4) { sh = 10; sm = 10;  return; }

    if (type == ScheduleType::WINTER) {
        if (start_class <= 6) { sh = 14; sm = 0;   return; }
        if (start_class <= 8) { sh = 15; sm = 55;  return; }
        if (start_class <= 10){ sh = 18; sm = 30;  return; }
        /* 11-12 */           sh = 20; sm = 15;  return;
    } else {
        if (start_class <= 6) { sh = 14; sm = 30;  return; }
        if (start_class <= 8) { sh = 16; sm = 25;  return; }
        if (start_class <= 10){ sh = 19; sm = 0;   return; }
        /* 11-12 */           sh = 20; sm = 45;  return;
    }
}

// 计算距离下课的剩余分钟（若已过，则返回0），以及格式化的下课时间字符串
static int calc_minutes_left_and_end_str(const std::tm& today, const Schedule& course, std::string& end_str_out)
{
    std::tm end_tm = today;
    ScheduleType type = ScheduleReminder::get_schedule_type(today);
    int eh = 0, em = 0;
    get_course_end_clock(type, course.get_end_class(), eh, em);
    end_tm.tm_hour = eh;
    end_tm.tm_min = em;
    end_tm.tm_sec = 0;
    end_tm.tm_isdst = -1;

    std::time_t now_tt = std::mktime(const_cast<std::tm*>(&today));
    std::time_t end_tt = std::mktime(&end_tm);
    double diff_sec = std::difftime(end_tt, now_tt);
    if (diff_sec <= 0) {
        end_str_out = (eh < 10 ? "0" : "") + std::to_string(eh) + ":" + (em < 10 ? "0" : "") + std::to_string(em);
        return 0;
    }
    end_str_out = (eh < 10 ? "0" : "") + std::to_string(eh) + ":" + (em < 10 ? "0" : "") + std::to_string(em);
    return static_cast<int>(diff_sec / 60.0 + 0.5);
}

// 新增：获取“下一节课”的详细信息（课程+日期+距上课分钟+开始时刻）
// 返回 true 表示找到
static bool get_next_class_detail(const std::string& qq_number,
                                  const std::tm& now_tm,
                                  Schedule& out_course,
                                  std::tm& out_date,
                                  std::string& start_clock,
                                  int& minutes_to_start)
{
    for (int day_offset = 0; day_offset < 7; ++day_offset) {
        std::tm target_date = now_tm;
        target_date.tm_mday += day_offset;
        std::mktime(&target_date);

        auto courses = ScheduleReminder::get_courses_on_date(qq_number, target_date);
        if (courses.empty()) continue;

        // 按节次排序
        std::sort(courses.begin(), courses.end(), [](const Schedule& a, const Schedule& b) {
            return a.get_start_class() < b.get_start_class();
        });

        ScheduleType type = ScheduleReminder::get_schedule_type(target_date);

        for (const auto& c : courses) {
            // 构造开始时刻
            int sh = 0, sm = 0;
            get_course_start_clock(type, c.get_start_class(), sh, sm);

            std::tm start_tm = target_date;
            start_tm.tm_hour = sh;
            start_tm.tm_min = sm;
            start_tm.tm_sec = 0;
            start_tm.tm_isdst = -1;

            std::time_t now_tt = std::mktime(const_cast<std::tm*>(&const_cast<std::tm&>(now_tm)));
            std::time_t start_tt = std::mktime(&start_tm);
            double diff_sec = std::difftime(start_tt, now_tt);
            if (diff_sec <= 0) {
                continue; // 已经开始或过去，找下一门
            }

            out_course = c;
            out_date = target_date;
            start_clock = (sh < 10 ? "0" : "") + std::to_string(sh) + ":" + (sm < 10 ? "0" : "") + std::to_string(sm);
            minutes_to_start = static_cast<int>(diff_sec / 60.0 + 0.5);
            return true;
        }
    }
    return false;
}

// 获取群内所有绑定用户的上课状态
std::vector<ReplyRule> get_class_inquiry_rules() {
    std::vector<ReplyRule> rules;

    // 规则：@bot + "有谁在上课" → 查询当前群内上课状态
    rules.push_back(ReplyRule{
        [](const json& msg_data, const std::string& content) {
            return is_at_bot(msg_data) && content == u8"有谁在上课";
        },
        [](const std::string& group_id, const std::string&) -> std::string {
            // 仅允许“绑定群聊”的群查询
            auto qs = get_query_groups();
            if (qs.find(group_id) == qs.end()) {
                return u8"本群尚未绑定查询群功能，请发送「绑定群聊」。";
            }

            // 1) 读取已导入课表的所有用户
            auto all_schedules = ScheduleLoader::load_from_file(SCHEDULE_FILE);

            // 2) 从成员缓存中获取“当前群内的已记录成员”
            //    仅将“已导入课表”的成员纳入统计
            std::vector<std::string> group_users;
            auto cached_members = get_group_member_qqs(group_id);
            group_users.reserve(cached_members.size());
            for (const auto& qq : cached_members) {
                if (all_schedules.find(qq) != all_schedules.end()) {
                    group_users.push_back(qq);
                }
            }

            if (group_users.empty()) {
                return u8"本群暂无已导入课表的成员，或成员尚未在本群发言被记录。\n请先导入课表，并在本群发送一条消息后再试。";
            }

            // 3) 后续逻辑保持不变（计算当前节次、统计在上课/下一节）
            std::time_t now = std::time(nullptr);
            std::tm current_time{};
#if defined(_MSC_VER)
            localtime_s(&current_time, &now);
#else
            current_time = *std::localtime(&now);
#endif
            current_time.tm_isdst = -1;
            std::mktime(&current_time);

            int current_period = get_current_class_period();
            if (current_period == -1) {
                return u8"当前非上课时间（冬季：8:00-21:50 / 夏季：8:00-22:20）～";
            }

            struct InClassInfo {
                std::string qq;
                Schedule course;
                int minutes_left;
                std::string end_clock;
            };

            struct FreeInfo {
                std::string qq;
                Schedule course;
                std::tm date;
                int minutes_to_start;
                std::string start_clock;
            };

            std::vector<InClassInfo> in_class_infos;
            std::vector<FreeInfo> free_infos;

            for (const std::string& qq : group_users) {
                std::vector<Schedule> today_courses = ScheduleReminder::get_courses_on_date(qq, current_time);
                const Schedule* the_course = nullptr;

                for (const auto& course : today_courses) {
                    if (course.get_start_class() <= current_period && course.get_end_class() >= current_period) {
                        the_course = &course;
                        break;
                    }
                }

                if (the_course != nullptr) {
                    std::string end_clock;
                    int minutes_left = calc_minutes_left_and_end_str(current_time, *the_course, end_clock);
                    in_class_infos.push_back(InClassInfo{ qq, *the_course, minutes_left, end_clock });
                } else {
                    Schedule nc;
                    std::tm nd{};
                    std::string start_clock;
                    int minutes_to_start = 0;
                    if (get_next_class_detail(qq, current_time, nc, nd, start_clock, minutes_to_start)) {
                        free_infos.push_back(FreeInfo{ qq, nc, nd, minutes_to_start, start_clock });
                    } else {
                        FreeInfo none{ qq, Schedule(), std::tm{}, -1, "" };
                        free_infos.push_back(none);
                    }
                }
            }

            std::stringstream reply;
            reply << u8"📊 当前群内上课状态（" << current_time.tm_hour << ":"
                  << std::setfill('0') << std::setw(2) << current_time.tm_min << u8"）：\n\n";

            if (!in_class_infos.empty()) {
                reply << u8"🎯 正在上课的用户：\n";
                for (size_t i = 0; i < in_class_infos.size(); ++i) {
                    const auto& info = in_class_infos[i];
                    int hours = info.minutes_left / 60;
                    int mins  = info.minutes_left % 60;
                    reply << u8"  " << i + 1 << ". "
                          << u8"成员：" << get_display_name(group_id, info.qq)
                          << u8" | 课程：" << info.course.get_name()
                          << u8"（第" << info.course.get_start_class() << u8"-" << info.course.get_end_class() << u8"节）"
                          << u8" | 下课：" << info.end_clock
                          << u8" | 剩余：" << (hours > 0 ? std::to_string(hours) + u8"小时" : "")
                          << std::to_string(mins) << u8"分钟"
                          << "\n";
                }
            } else {
                reply << u8"🎯 正在上课的用户：无\n";
            }
            reply << "\n";

            if (!free_infos.empty()) {
                reply << u8"⏰ 暂无课程的用户及下一节：\n";
                size_t idx = 1;
                for (const auto& fi : free_infos) {
                    reply << u8"  " << idx++ << ". " << get_display_name(group_id, fi.qq) << u8"：";
                    if (fi.minutes_to_start >= 0) {
                        char date_buf[32]{};
                        std::strftime(date_buf, sizeof(date_buf), "%m-%d", &fi.date);
                        std::string week_str = u8"周" + std::to_string(fi.date.tm_wday == 0 ? 7 : fi.date.tm_wday);

                        ScheduleType t = ScheduleReminder::get_schedule_type(fi.date);
                        std::string time_range;
                        if (fi.course.get_start_class() <= 4) {
                            time_range = (fi.course.get_start_class() <= 2) ? u8"8:00-9:40" : u8"10:10-11:50";
                        } else {
                            if (t == ScheduleType::WINTER) {
                                switch (fi.course.get_start_class()) {
                                case 5: time_range = u8"14:00-15:35"; break;
                                case 7: time_range = u8"15:55-17:30"; break;
                                case 9: time_range = u8"18:30-20:05"; break;
                                case 11: time_range = u8"20:15-21:50"; break;
                                default: time_range = u8"未知时段";
                                }
                            } else {
                                switch (fi.course.get_start_class()) {
                                case 5: time_range = u8"14:30-16:05"; break;
                                case 7: time_range = u8"16:25-17:50"; break;
                                case 9: time_range = u8"19:00-20:35"; break;
                                case 11: time_range = u8"20:45-22:20"; break;
                                default: time_range = u8"未知时段";
                                }
                            }
                        }

                        int h = fi.minutes_to_start / 60;
                        int m = fi.minutes_to_start % 60;

                        reply << u8"下一节：" << fi.course.get_name()
                              << u8"（" << date_buf << week_str << u8"）"
                              << u8" 第" << fi.course.get_start_class() << u8"-" << fi.course.get_end_class() << u8"节 "
                              << time_range
                              << u8" | 开始：" << fi.start_clock
                              << u8" | 距上课：" << (h > 0 ? std::to_string(h) + u8"小时" : "") << std::to_string(m) << u8"分钟";
                    } else {
                        reply << u8"未来7天暂无课程安排～";
                    }
                    reply << "\n";
                }
            } else {
                reply << u8"⏰ 暂无课程的用户：无\n";
            }

            return reply.str();
        }
        });

    return rules;
}