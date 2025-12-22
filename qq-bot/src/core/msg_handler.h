#ifndef GROUP_MSG_H
#define GROUP_MSG_H

#include <nlohmann/json.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>

using json = nlohmann::json;
namespace websocket = boost::beast::websocket;
using tcp_socket = boost::asio::ip::tcp::socket;

// 处理群消息（核心业务逻辑）
void handle_group_message(const json& msg_data, websocket::stream<tcp_socket>& ws);
//接受关键词并且回复
bool generate_reply(const json& msg_data, const std::string& trimmed_msg, const std::string& group_id, json& reply);

// 获取当前正在处理消息的 sender_qq（线程局部）
const std::string& get_current_sender_qq();

#endif // GROUP_MSG_H#pragma once
