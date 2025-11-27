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
        if (rule.matcher(msg_data, content)) {
            reply = {
                {"action", "send_group_msg"},
                {"params", {
                    {"group_id", group_id},
                    {"message", rule.reply_generator(group_id, content)}
                }}
            };
            return true;
        }
    }
    return false;
}