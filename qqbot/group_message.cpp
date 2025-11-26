// group_message.cpp：实现群消息处理逻辑
#include "group_message.h"
#include "utils.h"   // 调用工具函数（trim_space、is_at_bot、write_log）
#include "config.h"  // 用到机器人QQ
#include <iostream>
#include <boost/asio/buffer.hpp> // 替换为 Asio 的 buffer

// 实现群消息处理功能
void handle_group_message(const json& msg_data, websocket::stream<boost::asio::ip::tcp::socket>& ws) {
    try {
        // 提取群号、原始消息
        std::string group_id = std::to_string( msg_data["group_id"].get<long long>());
        // 提取消息前先判断字段是否存在
        if (!msg_data.contains("raw_message") || !msg_data["raw_message"].is_string()) {
            write_log("消息中没有raw_message字段，或字段类型不是字符串");
            return;
        }
        std::string raw_msg = msg_data["raw_message"].get<std::string>();
        std::string converted_msg = gbk_to_utf8(raw_msg);
        if (converted_msg.empty()) {
            write_log("编码转换失败，原始消息：" + raw_msg);
            return; // 转换失败则跳过处理
        }
        raw_msg = converted_msg;
        std::string trimmed_msg = trim_space(raw_msg); // 调用工具函数去空格

        // 写日志（调用工具函数）
        std::string log_content = "收到群" + group_id + "的消息：" + raw_msg;
        write_log(log_content);
        std::cout << log_content << std::endl;

        // 先判断是否 @ 机器人（并在需要时从消息里去除 at 部分再做 "1" 检测）
        bool at_bot = is_at_bot(msg_data);

        // 构造用于比较的消息文本（去掉 at 段）
        std::string compare_msg = trimmed_msg;
        if (at_bot && msg_data.contains("message") && msg_data["message"].is_array()) {
            std::string without_at;
            for (const auto& elem : msg_data["message"]) {
                // 保留纯文本片段，跳过 type == "at"
                const std::string type = elem.value("type", std::string());
                if (type == "text") {
                    // 常见格式：elem["data"]["text"]
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
                    // 对于其它类型（如可能的 plain 字段），尝试拼接可用文本字段
                    if (elem.contains("data") && elem["data"].is_object()) {
                        // 常见 key 为 "text"
                        without_at += elem["data"].value("text", std::string());
                    }
                }
            }
            compare_msg = trim_space(without_at);
        }

        // 功能1：@机器人 + 去掉@后消息是“1” → 回复“true”
        if (at_bot && compare_msg == "1") {
            json reply = {
                {"action", "send_group_msg"},
                {"params", {{"group_id", group_id}, {"message", "true"}}}
            };
            ws.write(boost::asio::buffer(reply.dump())); // 使用 Asio 的 buffer
            write_log("回复群" + group_id + "：true");
            std::cout << "回复成功：true" << std::endl;
        }

        // 功能2：关键词回复（“你好”→“哈喽～ 我是机器人！”）
        else if (raw_msg.find("hello") != std::string::npos) { // 检测消息是否包含“你好”
            json reply = {
                {"action", "send_group_msg"},
                {"params", {{"group_id", group_id}, {"message", "哈喽～ 我是机器人！"}}} };
            ws.write(boost::asio::buffer(reply.dump())); // 使用 Asio 的 buffer
            write_log("回复群" + group_id + "：哈喽～ 我是机器人！");
            std::cout << "关键词回复成功" << std::endl;
        }

        // 其他情况：不回复
        else {
            std::cout << "不满足回复条件，忽略" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::string err_log = "处理群消息失败：" + std::string(e.what());
        write_log(err_log);
        std::cerr << err_log << std::endl;
    }
}