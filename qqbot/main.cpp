// main.cpp: Main entry point for the QQ bot, handles NapCat message loop
#include <iostream>
#include "boost/asio.hpp"
#include "boost/beast.hpp"
#include "nlohmann/json.hpp"
#include "config.h"
#include "utils.h"
#include "group_message.h"

using namespace boost::asio;
using namespace boost::beast;
using json = nlohmann::json;
using tcp = boost::asio::ip::tcp;

int main() {
    try {
        // Write startup log
        write_log("Bot starting, connecting to NapCat...");
        std::cout << "========================================" << std::endl;
        std::cout << "Starting bot... Bot QQ: " << BOT_QQ << std::endl;
        std::cout << "Connecting to NapCat WS: " << WS_HOST << ":" << WS_PORT << std::endl;
        std::cout << "========================================" << std::endl;

        // 1. Initialize WebSocket connection
        io_context ioc;
        tcp::resolver resolver(ioc);
        websocket::stream<tcp::socket> ws(ioc);

        // 2. Resolve address and connect
        auto results = resolver.resolve(WS_HOST, WS_PORT);
        net::connect(ws.next_layer(), results.begin(), results.end());

        // 3. WebSocket handshake
        ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(http::field::user_agent, "C++ QQ Bot (v1)");
            }
        ));
        ws.handshake(WS_HOST, "/");

        // Connection success log
        std::string success_log = "WebSocket connected successfully, starting to listen for group messages...";
        write_log(success_log);
        std::cout << success_log << std::endl;

        // 4. Main message loop: receive messages and call group_message handler
        flat_buffer buffer;
        while (true) {
            ws.read(buffer);
            std::string msg = buffers_to_string(buffer.data());
            buffer.consume(buffer.size()); // Clear buffer

            // Parse message
            json msg_data = json::parse(msg);

            // Only handle group messages using group_message.h handler
            if (msg_data["post_type"] == "message" && msg_data["message_type"] == "group") {
                handle_group_message(msg_data, ws);
            }
            else {
                write_log("Received non-group message, ignoring: " + msg_data.dump(2));
            }
        }

        // Close connection (not actually reached due to infinite loop)
        ws.close(websocket::close_code::normal);
    }
    catch (const std::exception& e) {
        std::string err_log = "Exception occurred: " + std::string(e.what());
        write_log(err_log);
        std::cerr << "========================================" << std::endl;
        std::cerr << err_log << std::endl;
        std::cerr << "Possible cause: NapCat not running or port incorrect" << std::endl;
        std::cerr << "========================================" << std::endl;
        return 1;
    }

    return 0;
}
