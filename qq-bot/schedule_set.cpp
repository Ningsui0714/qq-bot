#include "reply_generator.h"
#include "config.h"
#include "utils.h"
#include "schedule.h"
#include "schedule_loader.h"
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

// 存储全局课表（内存+文件持久化）
static std::vector<Schedule> global_schedules;
// 课表持久化文件路径
const std::string SCHEDULE_FILE = "persistent_schedules.json";

// 初始化：启动时从文件加载课表
void init_schedules() {
    try {
        global_schedules = ScheduleLoader::load_from_file(SCHEDULE_FILE);
        write_log("Loaded " + std::to_string(global_schedules.size()) + " schedules from file");
    }
    catch (const std::exception& e) {
        write_log("No existing schedule file, start with empty list: " + std::string(e.what()));
        global_schedules.clear();
    }
}

// 持久化：保存课表到文件
bool save_schedules_to_file() {
    bool success = ScheduleLoader::save_to_file(global_schedules, SCHEDULE_FILE);
    if (success) {
        write_log("Schedules saved to file, total: " + std::to_string(global_schedules.size()));
    }
    else {
        write_log("Failed to save schedules to file");
    }
    return success;
}

// 解析逗号分隔的课程字符串：格式 "课程名,开始周,结束周,开始节,结束节"
bool parse_course_str(const std::string& content, Schedule& out_schedule) {
    std::vector<std::string> parts;
    std::stringstream ss(content);
    std::string part;

    // 按逗号分割字符串
    while (std::getline(ss, part, ',')) {
        part = trim_space(part);
        if (!part.empty()) {
            parts.push_back(part);
        }
    }

    // 校验格式（必须5个字段）
    if (parts.size() != 5) {
        return false;
    }

    try {
        // 解析数值字段
        int start_week = std::stoi(parts[1]);
        int end_week = std::stoi(parts[2]);
        int start_class = std::stoi(parts[3]);
        int end_class = std::stoi(parts[4]);

        // 校验数值合法性
        if (start_week < 1 || end_week < start_week || start_class < 1 || end_class < start_class) {
            return false;
        }

        // 构造课表对象
        out_schedule = Schedule(start_week, end_week, start_class, end_class, parts[0]);
        return true;
    }
    catch (...) {
        // 数值转换失败
        return false;
    }
}

std::vector<ReplyRule> Schedule::get_schedule_rules() {
    // 启动时初始化课表（仅执行一次）
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
            [](const std::string& group_id, const std::string&) -> std::string {
                return u8"请发送逗号分隔的课程信息，格式：\n课程名,开始周,结束周,开始节,结束节\n示例：高等数学,1,16,1,2（必须使用英文逗号）";
            }
        },
        // 规则2：@机器人 + "查询课表" → 展示所有课表
        ReplyRule{
            [](const json& msg_data, const std::string& content) {
                return is_at_bot(msg_data) && content == u8"查询课表";
            },
            [](const std::string& group_id, const std::string&) -> std::string {
                if (global_schedules.empty()) return u8"暂无已导入的课表，请按格式导入！";
                std::string reply = u8"已导入课表（共" + std::to_string(global_schedules.size()) + u8"门）：\n";
                for (size_t i = 0; i < global_schedules.size(); ++i) {
                    reply += std::to_string(i + 1) + ". " + global_schedules[i].to_string() + "\n";
                }
                return reply;
            }
        },
        // 规则3：解析逗号分隔的课程数据（自动导入，无需@）
        ReplyRule{
            [](const json& msg_data, const std::string& content) {
            // 快速过滤：必须包含4个逗号（5个字段）
            return std::count(content.begin(), content.end(), ',') == 4;
        },
        [](const std::string& group_id, const std::string& content) -> std::string {
            Schedule new_schedule;
            if (!parse_course_str(content, new_schedule)) {
                return u8"导入失败！请按正确格式输入：\n课程名,开始周,结束周,开始节,结束节\n示例：高等数学,1,16,1,2（必须使用英文逗号）";
            }

            // 添加到全局课表并持久化
            global_schedules.push_back(new_schedule);
            save_schedules_to_file();

            return u8"课表导入成功！\n" + new_schedule.to_string() + u8"\n发送“查询课表”查看全部";
        }
    },
        // 规则4：@机器人 + "清空课表" → 清空所有课表（可选扩展）
        ReplyRule{
            [](const json& msg_data, const std::string& content) {
                return is_at_bot(msg_data) && content == u8"清空课表";
            },
            [](const std::string& group_id, const std::string&) -> std::string {
                global_schedules.clear();
                save_schedules_to_file();
                return u8"所有课表已清空！";
            }
        }
    };
}