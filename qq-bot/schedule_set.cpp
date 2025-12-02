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

// 解析单条课程字符串：格式 "课程名，星期，开始周，结束周，开始节，结束节"
// 优先使用中文逗号“，”分割，兼容英文逗号“,”
bool parse_course_str(const std::string& content, Schedule& out_schedule, const std::string& qq_number) {
    auto replace_all = [](std::string& s, const std::string& from, const std::string& to) {
        if (from.empty()) return;
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
    };

    auto split_parts = [&](const std::string& s) {
        const std::string cn_comma = u8"，";
        std::vector<std::string> parts;
        std::string normalized = s;

        // 如果包含中文逗号，用中文逗号替换为英文逗号；否则直接用英文逗号
        if (normalized.find(cn_comma) != std::string::npos) {
            replace_all(normalized, cn_comma, ",");
        }

        std::stringstream ss(normalized);
        std::string part;
        while (std::getline(ss, part, ',')) {
            part = trim_space(part);
            if (!part.empty()) parts.push_back(part);
        }
        return parts;
    };

    std::vector<std::string> parts = split_parts(content);
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

// 将导入文本拆分为多条记录，支持换行或中文分号“；”分隔
static std::vector<std::string> split_records(const std::string& content) {
    const std::string cn_semicolon = u8"；";
    std::vector<std::string> records;
    std::string current;
    current.reserve(content.size());

    for (size_t i = 0; i < content.size(); ++i) {
        char ch = content[i];
        if (ch == '\r') continue; // 忽略 \r

        // 按 UTF-8 子串匹配中文分号
        if (content.compare(i, cn_semicolon.size(), cn_semicolon) == 0) {
            std::string trimmed = trim_space(current);
            if (!trimmed.empty()) records.push_back(trimmed);
            current.clear();
            i += cn_semicolon.size() - 1; // 跳过中文分号的剩余字节
        }
        else if (ch == '\n') {
            std::string trimmed = trim_space(current);
            if (!trimmed.empty()) records.push_back(trimmed);
            current.clear();
        }
        else {
            current.push_back(ch);
        }
    }
    std::string trimmed = trim_space(current);
    if (!trimmed.empty()) records.push_back(trimmed);
    return records;
}

// 判断是否可能为“导入课表”消息：至少包含一条记录且每条记录包含5个中文逗号（或兼容英文逗号）
static bool is_course_import_message(const std::string& content) {
    auto count_substr = [](const std::string& s, const std::string& sub) {
        size_t count = 0;
        size_t pos = 0;
        while ((pos = s.find(sub, pos)) != std::string::npos) {
            ++count;
            pos += sub.size();
        }
        return count;
    };

    const std::string cn_comma = u8"，";
    auto records = split_records(content);
    if (records.empty()) return false;
    for (const auto& rec : records) {
        size_t cn_commas = count_substr(rec, cn_comma);
        size_t en_commas = std::count(rec.begin(), rec.end(), ',');
        if ((cn_commas == 5) || (en_commas == 5)) {
            return true;
        }
    }
    return false;
}

std::vector<ReplyRule> Schedule::get_schedule_rules() {
    static bool initialized = false;
    if (!initialized) {
        init_schedules();
        initialized = true;
    }

    return {
        // 规则1：@机器人 + "导入课表" → 提示格式（中文逗号）
        ReplyRule{
            [](const json& msg_data, const std::string& content) {
                return is_at_bot(msg_data) && content == u8"导入课表";
            },
            [](const std::string&, const std::string&) -> std::string {
                return u8"请发送用中文逗号分隔的课程信息，格式：\n课程名，星期，开始周，结束周，开始节，结束节\n支持一次发送多条，使用换行或中文分号“；”分隔\n示例：高等数学，1，1，16，1，2";
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
        // 规则3：自动导入（无需@），支持中文逗号与批量导入
        ReplyRule{
            [](const json&, const std::string& content) {
                return is_course_import_message(content);
            },
            [](const std::string&, const std::string& content) -> std::string {
                const std::string& sender_qq = get_current_sender_qq();
                auto records = split_records(content);

                size_t success_count = 0;
                size_t fail_count = 0;
                std::string last_success_str;

                for (const auto& rec : records) {
                    Schedule new_schedule;
                    if (parse_course_str(rec, new_schedule, sender_qq)) {
                        global_schedules[sender_qq].push_back(new_schedule);
                        last_success_str = new_schedule.to_string();
                        ++success_count;
                    } else {
                        ++fail_count;
                    }
                }

                if (success_count > 0) {
                    save_schedules_to_file();
                }

                if (success_count == 0) {
                    return u8"导入失败！请使用中文逗号分隔：课程名，星期，开始周，结束周，开始节，结束节\n支持多条：用换行或中文分号“；”分隔\n示例：高等数学，1，1，16，1，2";
                }

                std::stringstream reply;
                reply << u8"课表导入成功 " << success_count << u8" 条";
                if (fail_count > 0) {
                    reply << u8"，失败 " << fail_count << u8" 条";
                }
                reply << u8"！\n";
                if (!last_success_str.empty()) {
                    reply << last_success_str << u8"\n";
                }
                reply << u8"发送“查询课表”查看全部";
                return reply.str();
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
                    return u8"学期开始日期已设置为：" + date_str + u8"（格式：YYYY-MM-DD）";
                }
                return u8"设置失败！请使用格式：设置学期 YYYY-MM-DD";
            }
        }
    };
}
