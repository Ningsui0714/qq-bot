#include "reply_generator.h"
#include "config.h"
#include "utils.h"

// 内置默认规则（原 keyword_rules 迁移至此，保持原有功能）
static const std::vector<ReplyRule> default_rules = {
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