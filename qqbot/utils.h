// utils.h: Common utility functions for the bot
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Function 1: Remove all spaces from a string (e.g., " 1 " -> "1")
std::string trim_space(const std::string& s);

// Function 2: Check if the message contains @bot mention
bool is_at_bot(const json& msg_data);

// Function 3: Write log to file with timestamp
void write_log(const std::string& content);

// Function 4: Ensure string is valid UTF-8 (sanitize invalid bytes)
std::string ensure_utf8(const std::string& input);

#endif // UTILS_H
