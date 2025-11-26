// group_message.cpp：实现群消息处理逻辑
#include "group_message.h"
#include "utils.h"
#include "config.h"
#include <iostream>
#include <boost/asio/buffer.hpp>

// 实现群消息处理函数
void handle_group_message(const json& msg_data, websocket::stream<boost::asio::ip::tcp::socket>& ws) {
    try {
        // 获取群号、原始消息
        std::string group_id = std::to_string(msg_data["group_id"].get<long long>());
        
        // 获取消息前先判断字段是否存在
        if (!msg_data.contains("raw_message") || !msg_data["raw_message"].is_string()) {
            write_log("消息中没有raw_message字段，或字段类型不是字符串");
            return;
        }
        
        std::string raw_msg = msg_data["raw_message"].get<std::string>();
        
        // 确保消息是有效的UTF-8编码（如有必要进行清理）
        raw_msg = ensure_utf8(raw_msg);
        
        std::string trimmed_msg = trim_space(raw_msg);

        // 写日志
        std::string log_content = "收到群" + group_id + "的消息：" + raw_msg;
        write_log(log_content);
        std::cout << log_content << std::endl;

        // 先判断是否@机器人
        bool at_bot = is_at_bot(msg_data);

        // 用于比较的消息文本（去掉at段）
        std::string compare_msg = trimmed_msg;
        if (at_bot && msg_data.contains("message") && msg_data["message"].is_array()) {
            std::string without_at;
            for (const auto& elem : msg_data["message"]) {
                // 只处理文本片段，跳过type == "at"
                const std::string type = elem.value("type", std::string());
                if (type == "text") {
                    // 标准格式：elem["data"]["text"]
                    if (elem.contains("data")) {
                        if (elem["data"].is_object()) {
                            without_at += elem["data"].value("text", std::string());
                        }
                        else if (elem["data"].is_string()) {
                            without_at += elem["data"].get<std::string>();
                        }
                    }
                }
                else if (type != "at") {
                    // 其他类型（可能的plain字段），尝试拼接可能的文本字段
                    if (elem.contains("data") && elem["data"].is_object()) {
                        without_at += elem["data"].value("text", std::string());
                    }
                }
            }
            compare_msg = trim_space(without_at);
        }

        // 情况1：@机器人 + 去掉@后消息是"1" -> 回复"true"
        if (at_bot && compare_msg == "1") {
            json reply = {
                {"action", "send_group_msg"},
                {"params", {{"group_id", group_id}, {"message", "true"}}}
            };
            ws.write(boost::asio::buffer(reply.dump()));
            write_log("回复群" + group_id + "：true");
            std::cout << "回复成功：true" << std::endl;
        }

        // 情况2：关键词回复（包含"hello"）
        else if (raw_msg.find("hello") != std::string::npos) {
            json reply = {
                {"action", "send_group_msg"},
                {"params", {{"group_id", group_id}, {"message", "你好！我是机器人！"}}}
            };
            ws.write(boost::asio::buffer(reply.dump()));
            write_log("回复群" + group_id + "：你好！我是机器人！");
            std::cout << "关键词回复成功" << std::endl;
        }

        // 其他情况：不回复
        else {
            std::cout << "不满足回复条件，忽略..." << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::string err_log = "处理群消息失败：" + std::string(e.what());
        write_log(err_log);
        std::cerr << err_log << std::endl;
    }
}
