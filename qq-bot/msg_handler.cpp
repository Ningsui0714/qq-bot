#include "msg_handler.h"
#include "utils.h"
#include "config.h"
#include <iostream>
#include <boost/asio/buffer.hpp>
#include <vector>
#include <functional>

struct KeywordRule {
    std::function<bool(const json&, const std::string&)> matcher;
    std::function<std::string(const std::string&)> reply_generator;
};

// 规范化文本：去除常见不可见字符、BOM、NBSP、全角空格，最终再 trim
static std::string normalize_text(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        if (c < 32 || c == 127) continue; // 去除ASCII控制字符
        out.push_back(static_cast<char>(c));
    }
    auto erase_seq = [&](const char* seq, size_t len) {
        for (;;) {
            auto pos = out.find(std::string(seq, len));
            if (pos == std::string::npos) break;
            out.erase(pos, len);
        }
    };
    // U+FEFF BOM, U+200B 零宽空格, U+00A0 NBSP, U+3000 全角空格
    erase_seq("\xEF\xBB\xBF", 3);
    erase_seq("\xE2\x80\x8B", 3);
    erase_seq("\xC2\xA0", 2);
    erase_seq("\xE3\x80\x80", 3);
    return trim_space(out);
}

// 关键词规则：所有回复改为英文
const std::vector<KeywordRule> keyword_rules = {
    {
        [](const json& msg_data, const std::string& trimmed_msg) {
            return is_at_bot(msg_data) && trimmed_msg == "1";
        },
        [](const std::string& group_id) {
            return "true";
        }
    },
    {
        [](const json& msg_data, const std::string& trimmed_msg) {
            return is_at_bot(msg_data) && trimmed_msg.find("hello") != std::string::npos;
        },
        [](const std::string& group_id) {
            return "Hello! I received your 'hello'~";
        }
    },
    // 使用 UTF-8 字面量，避免源码编码与运行时UTF-8不一致
    {
        [](const json& msg_data, const std::string& trimmed_msg) {
            return is_at_bot(msg_data) && trimmed_msg.find(u8"你好") != std::string::npos;
        },
        [](const std::string& group_id) {
            return u8"你好你好~";
        }
    }
};

bool generate_reply(const json& msg_data, const std::string& trimmed_msg, const std::string& group_id, json& reply) {
    for (const auto& rule : keyword_rules) {
        if (rule.matcher(msg_data, trimmed_msg)) {
            reply = {
                {"action", "send_group_msg"},
                {"params", {{"group_id", group_id}, {"message", rule.reply_generator(group_id)}}}
            };
            return true;
        }
    }
    return false;
}

void handle_group_message(const json& msg_data, websocket::stream<tcp_socket>& ws) {
    try {
        if (!msg_data.contains("group_id") || !msg_data["group_id"].is_number()) {
            write_log("Ignore invalid message: No group_id or wrong type");
            return;
        }
        std::string group_id = std::to_string(msg_data["group_id"].get<long long>());

        if (!msg_data.contains("raw_message") || !msg_data["raw_message"].is_string()) {
            write_log("Ignore invalid message: No raw_message or wrong type");
            return;
        }
        std::string raw_msg = msg_data["raw_message"].get<std::string>();

        std::string at_tag = "[CQ:at,qq=" + std::string(BOT_QQ) + "]";
        size_t at_pos = raw_msg.find(at_tag);
        if (at_pos != std::string::npos) {
            raw_msg = raw_msg.substr(at_pos + at_tag.length());
        }

        std::string trimmed_msg = normalize_text(raw_msg);

        write_log("Received message from group " + group_id + ": " + raw_msg + " (trimmed: " + trimmed_msg + ")");

        json reply;
        bool need_reply = generate_reply(msg_data, trimmed_msg, group_id, reply);

        if (need_reply) {
            std::string reply_str = reply.dump();
            ws.write(boost::asio::buffer(reply_str)); // 保持UTF-8发送
            write_log("Replied to group " + group_id + ": " + reply["params"]["message"].get<std::string>());
        } else {
            write_log("Group " + group_id + ": Message does not meet reply conditions, ignored");
        }
    }
    catch (const json::exception& e) {
        write_log("JSON error: " + std::string(e.what()) + ", Error id: " + std::to_string(e.id));
    }
    catch (const boost::beast::system_error& e) {
        write_log("WebSocket error: " + std::string(e.what()) + ", Error code: " + std::to_string(e.code().value()));
    }
    catch (const std::exception& e) {
        write_log("General error: " + std::string(e.what()));
    }
}
