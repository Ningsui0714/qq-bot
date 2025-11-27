#include "private_msg_handler.h"
#include "utils.h"
#include "config.h"
#include "reply_generator.h"
#include "guess_number.h"
#include "private_schedule.h"
#include <vector>

void handle_private_message(const json& msg_data, websocket::stream<tcp_socket>& ws) {
    try {
        // 存储当前消息数据（供个人课表模块使用）
        g_current_msg_data = msg_data;

        // 提取私聊专属字段（无group_id，避免访问不存在的键）
        std::string user_id = std::to_string(msg_data["user_id"].get<long long>());
        std::string raw_msg = msg_data["raw_message"].get<std::string>();
        std::string trimmed_msg = normalize_text(raw_msg);

        write_log("Private message processed (user: " + user_id + "): " + trimmed_msg);

        json reply;
        bool need_reply = false;
        std::string target_id = user_id; // 私聊目标ID即用户ID

        // 步骤1：猜数游戏规则（私聊支持）
        if (!need_reply) {
            std::vector<ReplyRule> guess_rules = get_guess_number_rules();
            // 适配猜数游戏的group_id参数（私聊用user_id替代，不影响逻辑）
            need_reply = ReplyGenerator::generate_with_rules(msg_data, trimmed_msg, target_id, guess_rules, reply);
        }

        // 步骤2：个人课表规则（核心私聊功能）
        if (!need_reply) {
            std::vector<ReplyRule> private_schedule_rules = PrivateScheduleManager::get_private_schedule_rules();
            need_reply = ReplyGenerator::generate_with_rules(msg_data, trimmed_msg, target_id, private_schedule_rules, reply);
        }

        // 发送私聊回复（固定用send_private_msg动作）
        if (need_reply) {
            reply["action"] = "send_private_msg";
            reply["params"]["user_id"] = user_id;
            reply["params"].erase("group_id"); // 确保删除群聊字段

            std::string reply_str = reply.dump();
            ws.write(boost::asio::buffer(reply_str));
            write_log("Replied to private user " + user_id + ": " + reply["params"]["message"].get<std::string>());
        }
        else {
            write_log("Private user " + user_id + ": No matching rule for message");
        }
    }
    catch (const json::exception& e) {
        write_log("Private message JSON error: " + std::string(e.what()) + ", Error id: " + std::to_string(e.id));
    }
    catch (const std::exception& e) {
        write_log("Private message handler error: " + std::string(e.what()));
    }
}