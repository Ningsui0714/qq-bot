// group_message.h: Group message handling declarations
#pragma once

#include "nlohmann/json.hpp"
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

using json = nlohmann::json;
using namespace boost::beast;
using tcp = boost::asio::ip::tcp;

// Handle group messages (business logic: @reply with true, keyword reply, etc.)
// Parameter 1: JSON message data from NapCat
// Parameter 2: WebSocket connection for sending replies
void handle_group_message(const json& msg_data, websocket::stream<tcp::socket>& ws);
