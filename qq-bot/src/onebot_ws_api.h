#pragma once
#include "utils.h"
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>

namespace websocket = boost::beast::websocket;
using tcp_socket = boost::asio::ip::tcp::socket;

// 初始化：保存 ws 指针供发送 action 使用（仅保存指针，不接管生命周期）
void onebot_api_init(websocket::stream<tcp_socket>* ws);

// 将收到的 WS 帧（JSON 已解析）投递给 API 层处理（用于处理带 echo 的回执）
void onebot_api_on_frame(const nlohmann::json& frame);

// 发送拉取群成员列表（异步，不阻塞当前线程）
void onebot_api_fetch_group_member_list(const std::string& group_id);