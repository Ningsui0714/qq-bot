#include "reply_generator.h"
#include "config.h"
#include "utils.h"
#include "schedule.h"
#include "schedule_loader.h"
#include "msg_handler.h"
#include "schedule_reminder.h"
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>

// 存储全局课表（内存+文件持久化，按 sender_qq 分类）
static std::map<std::string, std::vector<Schedule>> global_schedules;
// 课表持久化文件路径
const std::string SCHEDULE_FILE = "persistent_schedules.json";

// 初始化：启动时从文件加载课表（按 sender_qq）
void init_schedules() {
    try {
        global_schedules = ScheduleLoader::load_from_file(SCHEDULE_FILE);
        size_t total = 0;
        for (const auto& pair : global_schedules) {
            total += pair.second.size();
        }
        write_log("Loaded " + std::to_string(total) + " schedules for " +
            std::to_string(global_schedules.size()) + " senders");
    }
    catch (const std::exception& e) {
        write_log("No existing schedule file, start empty: " + std::string(e.what()));
        global_schedules.clear();
    }
}

// 持久化：保存课表到文件（按 sender_qq）
bool save_schedules_to_file() {
    bool success = ScheduleLoader::save_to_file(global_schedules, SCHEDULE_FILE);
    if (success) {
        size_t total = 0;
        for (const auto& pair : global_schedules) {
            total += pair.second.size();
        }
        write_log("Schedules saved, total: " + std::to_string(total) +
            " for " + std::to_string(global_schedules.size()) + " senders");
    }
    else {
        write_log("Failed to save schedules");
    }
    return success;
}

// 解析逗号分隔的课程字符串：格式 "课程名,星期,开始周,结束周,开始节,结束节"
bool parse_course_str(const std::string& content, Schedule& out_schedule, const std::string& qq_number) {
    std::vector<std::string> parts;
    std::stringstream ss(content);
    std::string part;
    while (std::getline(ss, part, ',')) {
        part = trim_space(part);
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    if (parts.size() != 6) {
        return false;
    }
    try {
        int weekday = std::stoi(parts[1]);
        int start_week = std::stoi(parts[2]);
        int end_week = std::stoi(parts[3]);
        int start_class = std::stoi(parts[4]);
        int end_class = std::stoi(parts[5]);
        if (weekday < 1 || weekday > 7 ||
            start_week < 1 || end_week < start_week ||
            start_class < 1 || end_class < start_class) {
            return false;
        }
        out_schedule = Schedule(start_week, end_week, start_class, end_class, weekday, parts[0], qq_number);
        return true;
    }
    catch (...) {
        return false;
    }
}

std::vector<ReplyRule> Schedule::get_schedule_rules() {
    static bool initialized = false;
    if (!initialized) {
        init_schedules();
        initialized = true;
    }

    return {
        // 规则1：@机器人 + "导入课表" → 提示格式
        ReplyRule{
            [](const json& msg_data, const std::string& content) {
                return is_at_bot(msg_data) && content == u8"导入课表";
            },
            [](const std::string&, const std::string&) -> std::string {
                return u8"请发送逗号分隔的课程信息，格式：\n课程名,星期,开始周,结束周,开始节,结束节\n示例：高等数学,1,1,16,1,2（必须使用英文逗号）";
            }
        },
        // 规则2：@机器人 + "查询课表" → 展示当前发送者课表（排序）
        ReplyRule{
            [](const json& msg_data, const std::string& content) {
                return is_at_bot(msg_data) && content == u8"查询课表";
            },
            [](const std::string&, const std::string&) -> std::string {
                const std::string& sender_qq = get_current_sender_qq();
                auto it = global_schedules.find(sender_qq);
                if (it == global_schedules.end() || it->second.empty())
                    return u8"你暂无已导入的课表，请按格式导入！";

                std::vector<Schedule> sorted = it->second;
                std::sort(sorted.begin(), sorted.end(), [](const Schedule& a, const Schedule& b) {
                    if (a.get_weekday() != b.get_weekday()) return a.get_weekday() < b.get_weekday();
                    if (a.get_start_class() != b.get_start_class()) return a.get_start_class() < b.get_start_class();
                    if (a.get_end_class() != b.get_end_class()) return a.get_end_class() < b.get_end_class();
                    return a.get_name() < b.get_name();
                });

                std::string reply = u8"你的课表（按星期、节次升序，共" + std::to_string(sorted.size()) + u8"门）：\n";
                for (size_t i = 0; i < sorted.size(); ++i) {
                    reply += std::to_string(i + 1) + ". " + sorted[i].to_string() + "\n";
                }
                return reply;
            }
        },
        // 规则3：自动导入（无需@）
        ReplyRule{
            [](const json&, const std::string& content) {
                return std::count(content.begin(), content.end(), ',') == 5;
            },
            [](const std::string&, const std::string& content) -> std::string {
                const std::string& sender_qq = get_current_sender_qq();
                Schedule new_schedule;
                if (!parse_course_str(content, new_schedule, sender_qq)) {
                    return u8"导入失败！格式：课程名,星期,开始周,结束周,开始节,结束节\n示例：高等数学,1,1,16,1,2";
                }
                global_schedules[sender_qq].push_back(new_schedule);
                save_schedules_to_file();
                return u8"课表导入成功！\n" + new_schedule.to_string() + u8"\n发送“查询课表”查看全部";
            }
        },
        // 规则4：@机器人 + "清空课表" → 清空当前发送者课表
        ReplyRule{
            [](const json& msg_data, const std::string& content) {
                return is_at_bot(msg_data) && content == u8"清空课表";
            },
            [](const std::string&, const std::string&) -> std::string {
                const std::string& sender_qq = get_current_sender_qq();
                auto it = global_schedules.find(sender_qq);
                if (it != global_schedules.end()) {
                    it->second.clear();
                }
                save_schedules_to_file();
                return u8"你的课表已清空！";
            }
        },
        // 规则5：@机器人 + "今日课程" → 返回今日课程提醒
        ReplyRule{
            [](const json& msg_data, const std::string& content) {
                return is_at_bot(msg_data) && content == u8"今日课程";
            },
            [](const std::string&, const std::string&) -> std::string {
                const std::string& sender_qq = get_current_sender_qq();
                return ScheduleReminder::get_today_courses_reminder(sender_qq);
            }
        },
        // 规则6： "设置学期 YYYY-MM-DD"（允许不@）
        ReplyRule{
            [](const json& msg_data, const std::string& content) {
                const std::string prefix = u8"设置学期";
                return !content.empty() && content.compare(0, prefix.size(), prefix) == 0;
            },
            [](const std::string&, const std::string& content) -> std::string {
                const std::string prefix = u8"设置学期";
                std::string date_str;
                if (content.size() > prefix.size()) {
                    date_str = content.substr(prefix.size());
                }
                date_str = trim_space(date_str);

                // 兼容中文或半角空格、大小写格式（如 2025-9-1 -> 2025-09-01）
                if (ScheduleReminder::set_term_start_date(date_str)) {
                    // 提示中返回用户输入的规范化结果（建议与 set_term_start_date 的规范化一致）
                    return u8"学期开始日期已设置为：" + date_str + u8"（格式：YYYY-MM-DD）";
                }
                return u8"设置失败！请使用格式：设置学期 YYYY-MM-DD";
            }
        }
    };
}
