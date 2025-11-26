// utils.cpp：实现utils.h声明的函数
#include "utils.h"
#include "config.h"
#include <algorithm>
#include <fstream>
#include <ctime>
#include <iostream>
#include <vector>
#include <cstring>

// 实现函数1：去除字符串空格
std::string trim_space(const std::string& s) {
    std::string res = s;
    res.erase(std::remove(res.begin(), res.end(), ' '), res.end());
    return res;
}

// 实现函数2：检查是否@机器人
bool is_at_bot(const json& msg_data) {
    try {
        if (msg_data.contains("message") && msg_data["message"].is_array()) {
            for (const auto& elem : msg_data["message"]) {
                // 对比@的QQ号和机器人的BOT_QQ
                if (elem["type"] == "at" && elem["data"]["qq"] == BOT_QQ) {
                    return true;
                }
            }
        }
        return false;
    }
    catch (const json::exception& e) {
        write_log("检查@机器人失败：" + std::string(e.what()));
        return false;
    }
}

// 实现函数3：写日志到文件
void write_log(const std::string& content) {
    // 获取当前时间并格式化为字符串（YYYY-MM-DD HH:MM:SS）
    std::time_t now = std::time(nullptr);
    char time_str[64] = "time_error";

#if defined(_MSC_VER)
    // MSVC：使用线程安全的localtime_s
    std::tm tm{};
    if (localtime_s(&tm, &now) == 0) {
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);
    }
#else
    // POSIX：使用线程安全的localtime_r
    std::tm tm{};
    if (localtime_r(&now, &tm) != nullptr) {
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);
    }
#endif

    // 打开日志文件（ios::app表示追加内容，不会覆盖旧日志）
    std::ofstream log_file(LOG_FILE, std::ios::app);
    if (log_file.is_open()) {
        log_file << "[" << time_str << "] " << content << std::endl;
        log_file.close();
    }
    else {
        std::cerr << "日志文件打开失败" << std::endl;
    }
}

// 辅助函数：验证字符串是否是有效的UTF-8编码
static bool is_valid_utf8(const std::string& s) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(s.data());
    size_t len = s.size();
    size_t i = 0;
    while (i < len) {
        unsigned char c = bytes[i];
        if (c <= 0x7F) { // ASCII字符
            i += 1;
        }
        else if ((c >> 5) == 0x6) { // 110x xxxx，2字节
            if (i + 1 >= len) return false;
            if ((bytes[i + 1] >> 6) != 0x2) return false;
            i += 2;
        }
        else if ((c >> 4) == 0xE) { // 1110 xxxx，3字节
            if (i + 2 >= len) return false;
            if ((bytes[i + 1] >> 6) != 0x2 || (bytes[i + 2] >> 6) != 0x2) return false;
            i += 3;
        }
        else if ((c >> 3) == 0x1E) { // 1111 0xxx，4字节
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

// 实现函数4：确保字符串是有效的UTF-8编码
// 如果已经是有效的UTF-8，直接返回；否则用'?'替换无效字节
std::string ensure_utf8(const std::string& input) {
    // 如果已经是有效的UTF-8，直接返回
    if (is_valid_utf8(input)) {
        return input;
    }
    
    // 用'?'替换无效的UTF-8字节
    std::string result;
    result.reserve(input.size());
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(input.data());
    size_t len = input.size();
    size_t i = 0;
    
    while (i < len) {
        unsigned char c = bytes[i];
        if (c <= 0x7F) { // ASCII字符
            result.push_back(static_cast<char>(c));
            i += 1;
        }
        else if ((c >> 5) == 0x6 && i + 1 < len && (bytes[i + 1] >> 6) == 0x2) {
            // 有效的2字节序列
            result.push_back(static_cast<char>(bytes[i]));
            result.push_back(static_cast<char>(bytes[i + 1]));
            i += 2;
        }
        else if ((c >> 4) == 0xE && i + 2 < len && 
                 (bytes[i + 1] >> 6) == 0x2 && (bytes[i + 2] >> 6) == 0x2) {
            // 有效的3字节序列
            result.push_back(static_cast<char>(bytes[i]));
            result.push_back(static_cast<char>(bytes[i + 1]));
            result.push_back(static_cast<char>(bytes[i + 2]));
            i += 3;
        }
        else if ((c >> 3) == 0x1E && i + 3 < len && 
                 (bytes[i + 1] >> 6) == 0x2 && (bytes[i + 2] >> 6) == 0x2 && 
                 (bytes[i + 3] >> 6) == 0x2) {
            // 有效的4字节序列
            result.push_back(static_cast<char>(bytes[i]));
            result.push_back(static_cast<char>(bytes[i + 1]));
            result.push_back(static_cast<char>(bytes[i + 2]));
            result.push_back(static_cast<char>(bytes[i + 3]));
            i += 4;
        }
        else {
            // 无效字节，用'?'替换
            result.push_back('?');
            i += 1;
        }
    }
    
    return result;
}
