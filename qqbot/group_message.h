// group_message.h：声明群消息相关功能
#pragma once

#include "nlohmann/json.hpp"
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

using json = nlohmann::json;
using namespace boost::beast;
using tcp = boost::asio::ip::tcp;

// 处理群消息（核心业务：@回复true、关键词回复）
// 参数1：NapCat发来的JSON消息；参数2：WebSocket连接（用来发回复）
void handle_group_message(const json& msg_data, websocket::stream<tcp::socket>& ws);
