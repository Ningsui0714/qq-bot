#pragma once
#ifndef PRIVATE_MSG_HANDLER_H
#define PRIVATE_MSG_HANDLER_H
#include <nlohmann/json.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp> // 修复：添加此头文件声明 tcp::socket
using json = nlohmann::json;
namespace websocket = boost::beast::websocket;
using tcp_socket = boost::asio::ip::tcp::socket;

// 处理私聊消息（专属逻辑，避免与群聊冲突）
void handle_private_message(const json& msg_data, websocket::stream<tcp_socket>& ws);

#endif // PRIVATE_MSG_HANDLER_H