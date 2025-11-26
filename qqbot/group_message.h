// group_message.h：群消息处理相关声明
#pragma once

#include "nlohmann/json.hpp"
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

using json = nlohmann::json;
using namespace boost::beast;
using tcp = boost::asio::ip::tcp;

// 处理群消息（业务逻辑：@回复true、关键词回复等）
// 参数1：NapCat发来的JSON消息数据
// 参数2：WebSocket连接，用于发送回复
void handle_group_message(const json& msg_data, websocket::stream<tcp::socket>& ws);
