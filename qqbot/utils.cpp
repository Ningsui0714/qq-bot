// utils.cpp: Implementation of utility functions
#include "utils.h"
#include "config.h"
#include <algorithm>
#include <fstream>
#include <ctime>
#include <iostream>
#include <vector>
#include <cstring>

// Function 1: Remove all spaces from a string
std::string trim_space(const std::string& s) {
    std::string res = s;
    res.erase(std::remove(res.begin(), res.end(), ' '), res.end());
    return res;
}

// Function 2: Check if the message contains @bot mention
bool is_at_bot(const json& msg_data) {
    try {
        if (msg_data.contains("message") && msg_data["message"].is_array()) {
            for (const auto& elem : msg_data["message"]) {
                // Compare @qq with BOT_QQ
                if (elem["type"] == "at" && elem["data"]["qq"] == BOT_QQ) {
                    return true;
                }
            }
        }
        return false;
    }
    catch (const json::exception& e) {
        write_log("Failed to check @bot: " + std::string(e.what()));
        return false;
    }
}

// Function 3: Write log to file with timestamp
void write_log(const std::string& content) {
    // Get current time and format it as string (YYYY-MM-DD HH:MM:SS)
    std::time_t now = std::time(nullptr);
    char time_str[64] = "time_error";

#if defined(_MSC_VER)
    // MSVC: Use thread-safe localtime_s
    std::tm tm{};
    if (localtime_s(&tm, &now) == 0) {
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);
    }
#else
    // POSIX: Use thread-safe localtime_r
    std::tm tm{};
    if (localtime_r(&now, &tm) != nullptr) {
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);
    }
#endif

    // Open log file in append mode
    std::ofstream log_file(LOG_FILE, std::ios::app);
    if (log_file.is_open()) {
        log_file << "[" << time_str << "] " << content << std::endl;
        log_file.close();
    }
    else {
        std::cerr << "Failed to open log file" << std::endl;
    }
}

// Helper function: Check if a string is valid UTF-8
static bool is_valid_utf8(const std::string& s) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(s.data());
    size_t len = s.size();
    size_t i = 0;
    while (i < len) {
        unsigned char c = bytes[i];
        if (c <= 0x7F) { // ASCII
            i += 1;
        }
        else if ((c >> 5) == 0x6) { // 110x xxxx, 2 bytes
            if (i + 1 >= len) return false;
            if ((bytes[i + 1] >> 6) != 0x2) return false;
            i += 2;
        }
        else if ((c >> 4) == 0xE) { // 1110 xxxx, 3 bytes
            if (i + 2 >= len) return false;
            if ((bytes[i + 1] >> 6) != 0x2 || (bytes[i + 2] >> 6) != 0x2) return false;
            i += 3;
        }
        else if ((c >> 3) == 0x1E) { // 1111 0xxx, 4 bytes
            if (i + 3 >= len) return false;
            if ((bytes[i + 1] >> 6) != 0x2 || (bytes[i + 2] >> 6) != 0x2 || (bytes[i + 3] >> 6) != 0x2) return false;
            i += 4;
        }
        else {
            return false;
        }
    }
    return true;
}

// Function 4: Ensure string is valid UTF-8 (sanitize invalid bytes)
// If already valid UTF-8, return as-is. Otherwise, replace invalid bytes with '?'
std::string ensure_utf8(const std::string& input) {
    // If already valid UTF-8, return as-is
    if (is_valid_utf8(input)) {
        return input;
    }
    
    // Sanitize invalid UTF-8 bytes by replacing them with '?'
    std::string result;
    result.reserve(input.size());
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(input.data());
    size_t len = input.size();
    size_t i = 0;
    
    while (i < len) {
        unsigned char c = bytes[i];
        if (c <= 0x7F) { // ASCII
            result.push_back(static_cast<char>(c));
            i += 1;
        }
        else if ((c >> 5) == 0x6 && i + 1 < len && (bytes[i + 1] >> 6) == 0x2) {
            // Valid 2-byte sequence
            result.push_back(static_cast<char>(bytes[i]));
            result.push_back(static_cast<char>(bytes[i + 1]));
            i += 2;
        }
        else if ((c >> 4) == 0xE && i + 2 < len && 
                 (bytes[i + 1] >> 6) == 0x2 && (bytes[i + 2] >> 6) == 0x2) {
            // Valid 3-byte sequence
            result.push_back(static_cast<char>(bytes[i]));
            result.push_back(static_cast<char>(bytes[i + 1]));
            result.push_back(static_cast<char>(bytes[i + 2]));
            i += 3;
        }
        else if ((c >> 3) == 0x1E && i + 3 < len && 
                 (bytes[i + 1] >> 6) == 0x2 && (bytes[i + 2] >> 6) == 0x2 && 
                 (bytes[i + 3] >> 6) == 0x2) {
            // Valid 4-byte sequence
            result.push_back(static_cast<char>(bytes[i]));
            result.push_back(static_cast<char>(bytes[i + 1]));
            result.push_back(static_cast<char>(bytes[i + 2]));
            result.push_back(static_cast<char>(bytes[i + 3]));
            i += 4;
        }
        else {
            // Invalid byte, replace with '?'
            result.push_back('?');
            i += 1;
        }
    }
    
    return result;
}
