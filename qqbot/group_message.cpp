// group_message.cpp: Implementation of group message handling logic
#include "group_message.h"
#include "utils.h"
#include "config.h"
#include <iostream>
#include <boost/asio/buffer.hpp>

// Implementation of group message handler
void handle_group_message(const json& msg_data, websocket::stream<boost::asio::ip::tcp::socket>& ws) {
    try {
        // Get group ID and original message
        std::string group_id = std::to_string(msg_data["group_id"].get<long long>());
        
        // Check if raw_message field exists before getting message
        if (!msg_data.contains("raw_message") || !msg_data["raw_message"].is_string()) {
            write_log("Message does not contain raw_message field or field type is not string");
            return;
        }
        
        std::string raw_msg = msg_data["raw_message"].get<std::string>();
        
        // Ensure message is valid UTF-8 (sanitize if needed)
        raw_msg = ensure_utf8(raw_msg);
        
        std::string trimmed_msg = trim_space(raw_msg);

        // Write log
        std::string log_content = "Received message from group " + group_id + ": " + raw_msg;
        write_log(log_content);
        std::cout << log_content << std::endl;

        // Check if message mentions @bot
        bool at_bot = is_at_bot(msg_data);

        // Get message text without @mention for comparison
        std::string compare_msg = trimmed_msg;
        if (at_bot && msg_data.contains("message") && msg_data["message"].is_array()) {
            std::string without_at;
            for (const auto& elem : msg_data["message"]) {
                // Only process text segments, skip type == "at"
                const std::string type = elem.value("type", std::string());
                if (type == "text") {
                    // Standard format: elem["data"]["text"]
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
                    // Other types (like plain text), try to extract text field
                    if (elem.contains("data") && elem["data"].is_object()) {
                        without_at += elem["data"].value("text", std::string());
                    }
                }
            }
            compare_msg = trim_space(without_at);
        }

        // Case 1: @bot + message is "1" -> reply "true"
        if (at_bot && compare_msg == "1") {
            json reply = {
                {"action", "send_group_msg"},
                {"params", {{"group_id", group_id}, {"message", "true"}}}
            };
            ws.write(boost::asio::buffer(reply.dump()));
            write_log("Replied to group " + group_id + ": true");
            std::cout << "Reply successful: true" << std::endl;
        }

        // Case 2: Keyword reply - when message contains "hello"
        else if (raw_msg.find("hello") != std::string::npos) {
            json reply = {
                {"action", "send_group_msg"},
                {"params", {{"group_id", group_id}, {"message", "Hello! I am a bot!"}}}
            };
            ws.write(boost::asio::buffer(reply.dump()));
            write_log("Replied to group " + group_id + ": Hello! I am a bot!");
            std::cout << "Keyword reply successful" << std::endl;
        }

        // Other cases: No reply
        else {
            std::cout << "No matching response, ignoring..." << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::string err_log = "Failed to handle group message: " + std::string(e.what());
        write_log(err_log);
        std::cerr << err_log << std::endl;
    }
}
