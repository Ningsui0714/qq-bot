#include "reply_generator.h"
#include "config.h"
#include "utils.h"

// 内置默认规则（原 keyword_rules 迁移至此，保持原有功能）
static const std::vector<ReplyRule> default_rules = {
    // 📖 功能总览（@机器人 + 帮助/功能/指令/空内容）
    {
        [](const json& msg_data, const std::string& content) {
            const bool at_me = is_at_bot(msg_data);
            const bool is_help = content.empty() || content == u8"帮助" || content == u8"功能" || content == u8"指令";
            return at_me && is_help;
        },
        [](const std::string& /*group_id*/) {
            std::string s;
            s += u8"📖 功能总览\n\n";
            s += u8"一、课表管理\n";
            s += u8"- @机器人 导入课表：获取导入格式说明；支持中文逗号\n";
            s += u8"- 直接发送课程多行文本：自动批量导入（换行或中文分号“；”分隔）\n";
            s += u8"- @机器人 查询课表：查看你已导入的全部课程\n";
            s += u8"- @机器人 清空课表：清空你的课程（注意目前无法删除单个课程）\n";
            s += u8"- @机器人 今日课程：查看你今天的课程提醒\n\n";
            s += u8"二、上课查询（群内）\n";
            s += u8"- @机器人 有谁在上课：统计当前群内成员的上课状态\n";
            s += u8"- 绑定群聊 / 取消绑定群聊：把本群启用/关闭“上课查询”功能\n\n";
            s += u8"三、提醒功能\n";
            s += u8"- 设置提醒群：将“你的个人提醒”绑定到本群（22:00 推送你的明日课程）\n";
            s += u8"- 绑定群提醒 / 取消绑定群提醒：设置/取消“全局唯一提醒群”（22:00 推送全体有课成员的明日课程）\n\n";
            s += u8"四、小游戏\n";
            s += u8"- @机器人 猜数：开始1-100猜数字游戏\n";
            s += u8"- @机器人 退出：结束当前群的猜数游戏\n\n";
            s += u8"提示\n";
            s += u8"- 以上指令均在群内使用；标注“@机器人”的需要先@本机器人\n";
            s += u8"- 发生异常或无响应时，可再次尝试或稍后重试\n";
            s += u8"- 当前所有指令只能在群聊中实现\n";
            s += u8"- 项目地址：https://github.com/Ningsui0714/qq-bot\n";
            return s;
        }
    },
    {
        [](const json& msg_data, const std::string& content) {
            return is_at_bot(msg_data) && content == "1";
        },
        [](const std::string& group_id) {
            return "true";
        }
    },
    {
        [](const json& msg_data, const std::string& content) {
            return is_at_bot(msg_data) && content == "hello";
        },
        [](const std::string& group_id) {
            return "Hello! I received your 'hello'~";
        }
    },
    {
        [](const json& msg_data, const std::string& content) {
            return is_at_bot(msg_data) && content == u8"你好";
        },
        [](const std::string& group_id) {
            return u8"你好你好~";
        }
    }
};

// 方式1：使用默认规则（兼容原有调用）
bool ReplyGenerator::generate(const json& msg_data, const std::string& content, const std::string& group_id, json& reply) {
    return generate_with_rules(msg_data, content, group_id, default_rules, reply);
}

// 方式2：使用自定义规则（核心通用逻辑）
bool ReplyGenerator::generate_with_rules(const json& msg_data, const std::string& content, const std::string& group_id,
    const std::vector<ReplyRule>& custom_rules, json& reply) {
    // 遍历规则，匹配成功则生成回复
    for (const auto& rule : custom_rules) {
        bool matched = false;
        try {
            matched = rule.matcher(msg_data, content);
        } catch (const std::exception& e) {
            write_log(std::string("Rule matcher threw: ") + e.what());
            matched = false;
        } catch (...) {
            write_log("Rule matcher threw: unknown exception");
            matched = false;
        }

        if (matched) {
            try {
                // 尝试取发送者QQ，用于统一@封装
                std::string sender_qq;
                try {
                    if (msg_data.contains("sender") && msg_data["sender"].contains("user_id")) {
                        sender_qq = std::to_string(msg_data["sender"]["user_id"].get<long long>());
                    }
                } catch (...) {
                    sender_qq.clear();
                }

                const std::string plain = rule.reply_generator(group_id, content);
                const std::string message = sender_qq.empty() ? plain : with_at(sender_qq, plain);

                reply = {
                    {"action", "send_group_msg"},
                    {"params", {
                        {"group_id", group_id},
                        {"message", message}
                    }}
                };
                return true;
            } catch (const std::exception& e) {
                write_log(std::string("Rule reply_generator threw: ") + e.what());
            } catch (...) {
                write_log("Rule reply_generator threw: unknown exception");
            }
        }
    }
    return false;
}