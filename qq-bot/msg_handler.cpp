#include "msg_handler.h"
#include "utils.h"
#include "config.h"
#include <iostream>
#include <boost/asio/buffer.hpp>
#include <vector>
#include <functional>
#include "reply_generator.h"
#include "guess_number.h"
#include "schedule.h"
#include "schedule_loader.h"
#include "group_mapping.h"
#include "class_inquiry.h"
#include "member_cache.h" // + 引入

// 线程局部保存当前 sender_qq，供规则内部调用
static thread_local std::string g_current_sender_qq;
const std::string& get_current_sender_qq() { return g_current_sender_qq; }

struct KeywordRule {
    std::function<bool(const json&, const std::string&)> matcher;
    std::function<std::string(const std::string&)> reply_generator;
};

static std::string decode_html_entities(const std::string& s)
{
    std::string out = s;
    auto replace_all = [&](const char* from, const char* to)
    {
        std::string f(from);
        std::string t(to);
        std::size_t pos = 0;
        while ((pos = out.find(f, pos)) != std::string::npos)
        {
            out.replace(pos, f.length(), t);
            pos += t.length();
        }
    };
    // 最常用的实体
    replace_all("&#91;", "[");   // 修正：原来是 "[]"
    replace_all("&#93;", "]");
    replace_all("&quot;", "\"");
    replace_all("&amp;", "&");
    replace_all("&#123;", "{");
    replace_all("&#125;", "}");
    return out;
}

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
    out = trim_space(out);
    out = decode_html_entities(out); // 追加实体解码
    return out;
}



void handle_group_message(const json& msg_data, websocket::stream<tcp_socket>& ws) {
    try {
        if (!msg_data.contains("group_id") || !msg_data["group_id"].is_number()) {
            write_log("Ignore invalid message: No group_id or wrong type");
            return;
        }
        std::string group_id = std::to_string(msg_data["group_id"].get<long long>());
        // 2. 验证并获取发送者QQ号（新增核心逻辑）
        std::string sender_qq;
        if (msg_data.contains("user_id") && msg_data["user_id"].is_number()) {
            // 直接获取user_id（大部分CQHTTP协议的字段）
            sender_qq = std::to_string(msg_data["user_id"].get<long long>());
        }
        else if (msg_data.contains("sender") && msg_data["sender"].is_object() &&
            msg_data["sender"].contains("user_id") && msg_data["sender"]["user_id"].is_number()) {
            // 兼容嵌套在sender对象中的情况
            sender_qq = std::to_string(msg_data["sender"]["user_id"].get<long long>());
        }
        else {
            write_log("Ignore invalid message: No sender QQ (user_id) or wrong type");
            return;
        }

        // 将当前 sender_qq 暴露给规则层
        g_current_sender_qq = sender_qq;

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

        write_log("Received message from group " + group_id + " qq号：" + sender_qq + ": " + raw_msg + " (trimmed: " + trimmed_msg + ")");

        json reply;
        bool need_reply = false;

        // 步骤1：先尝试默认规则（1/hello/你好）
        need_reply = ReplyGenerator::generate(msg_data, trimmed_msg, group_id, reply);

        // 步骤2：课表相关规则（导入/查询/清空/今日课程/设置学期 等）
        if (!need_reply) {
            std::vector<ReplyRule> schedule_rules = Schedule().get_schedule_rules();
            need_reply = ReplyGenerator::generate_with_rules(msg_data, trimmed_msg, group_id, schedule_rules, reply);
        }

        // 步骤3：尝试猜数游戏规则
        if (!need_reply) {
            std::vector<ReplyRule> guess_rules = get_guess_number_rules();
            need_reply = ReplyGenerator::generate_with_rules(msg_data, trimmed_msg, group_id, guess_rules, reply);
        }
        // 步骤4：尝试"有谁在上课"查询规则
        if (!need_reply) {
            std::vector<ReplyRule> class_inquiry_rules = get_class_inquiry_rules();
            need_reply = ReplyGenerator::generate_with_rules(msg_data, trimmed_msg, group_id, class_inquiry_rules, reply);
        }

        // 新增指令处理
        if (!need_reply) {
            if (trimmed_msg == "设置提醒群") {
                set_group_id_for_qq(sender_qq, group_id);
                json reply_msg = {
                    {"action", "send_group_msg"},
                    {"params", {
                        {"group_id", group_id},
                        {"message", "[CQ:at,qq=" + sender_qq + "] 已绑定此群为你的每日课程提醒群（22:00 推送明日课程）。"}
                    }}
                };
                reply = reply_msg;
                need_reply = true;
            } else if (trimmed_msg == "绑定群聊") {
                add_query_group(group_id);
                json reply_msg = {
                    {"action", "send_group_msg"},
                    {"params", {
                        {"group_id", group_id},
                        {"message", "✅ 已将本群绑定为查询群，可直接发送「有谁在上课」查看。"},
                        {"auto_escape", false}
                    }}
                };
                reply = reply_msg;
                need_reply = true;
            } else if (trimmed_msg == "取消绑定群聊" || trimmed_msg == "解绑群聊") {
                remove_query_group(group_id);
                json reply_msg = {
                    {"action", "send_group_msg"},
                    {"params", {
                        {"group_id", group_id},
                        {"message", "✅ 已取消本群的查询群绑定。"},
                        {"auto_escape", false}
                    }}
                };
                reply = reply_msg;
                need_reply = true;
            } else if (trimmed_msg == "绑定群提醒") {
                set_reminder_group(group_id);
                json reply_msg = {
                    {"action", "send_group_msg"},
                    {"params", {
                        {"group_id", group_id},
                        {"message", "✅ 已将本群设置为提醒群，将在每日22:00推送「明日课程」。"},
                        {"auto_escape", false}
                    }}
                };
                reply = reply_msg;
                need_reply = true;
            } else if (trimmed_msg == "取消绑定群提醒" || trimmed_msg == "解绑群提醒") {
                clear_reminder_group();
                json reply_msg = {
                    {"action", "send_group_msg"},
                    {"params", {
                        {"group_id", group_id},
                        {"message", "✅ 已取消提醒群设置。"},
                        {"auto_escape", false}
                    }}
                };
                reply = reply_msg;
                need_reply = true;
            }
        }

        // 发送
        if (need_reply) {
            std::string reply_str = reply.dump();
            ws.write(boost::asio::buffer(reply_str));
            write_log("Replied to group " + group_id + ": " + reply["params"]["message"].get<std::string>());
        }
        else {
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
    update_member_display_name(msg_data);
}