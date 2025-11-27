#include "utils.h"
#include "config.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <ctime>
#include <iomanip>

std::string gbk_to_utf8(const std::string& gbk_str) {
    int wide_len = MultiByteToWideChar(936, 0, gbk_str.c_str(), -1, nullptr, 0);
    if (wide_len == 0) {
        write_log("GBK to wide char failed, Error code: " + std::to_string(GetLastError()));
        return gbk_str;
    }
    std::wstring wide_str(wide_len, 0);
    MultiByteToWideChar(936, 0, gbk_str.c_str(), -1, &wide_str[0], wide_len);

    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8_len == 0) {
        write_log("Wide char to UTF-8 failed, Error code: " + std::to_string(GetLastError()));
        return gbk_str;
    }
    std::string utf8_str(utf8_len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, &utf8_str[0], utf8_len, nullptr, nullptr);
    utf8_str.pop_back();
    return utf8_str;
}

std::string utf8_to_gbk(const std::string& utf8_str) {
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (wide_len == 0) return utf8_str;
    std::wstring wide_str(wide_len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wide_str[0], wide_len);

    int gbk_len = WideCharToMultiByte(936, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (gbk_len == 0) return utf8_str;
    std::string gbk_str(gbk_len, 0);
    WideCharToMultiByte(936, 0, wide_str.c_str(), -1, &gbk_str[0], gbk_len, nullptr, nullptr);
    gbk_str.pop_back();
    return gbk_str;
}

std::string trim_space(const std::string& str) {
    if (str.empty()) return "";
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");
    return start == std::string::npos ? "" : str.substr(start, end - start + 1);
}

bool is_at_bot(const json& msg_data) {
    if (!msg_data.contains("raw_message") || !msg_data["raw_message"].is_string()) {
        return false;
    }
    try {
        std::string raw_msg = msg_data["raw_message"].get<std::string>();
        std::string trimmed_raw = trim_space(raw_msg); // 新增：去除首尾空格
        std::string at_tag = "[CQ:at,qq=" + std::string(BOT_QQ) + "]";
        // 同时检查原始消息和去除首尾空格后的消息，提高容错
        return raw_msg.find(at_tag) != std::string::npos
            || trimmed_raw.find(at_tag) != std::string::npos;
    }
    catch (const json::exception& e) {
        write_log("Parse @bot tag failed: " + std::string(e.what()));
        return false;
    }
}

void write_log(const std::string& content) {
    std::time_t now = std::time(nullptr);
    std::tm tm_now;
    localtime_s(&tm_now, &now);
    char time_buf[64];
    std::strftime(time_buf, sizeof(time_buf), "[%Y-%m-%d %H:%M:%S]", &tm_now);

    std::string log_content = std::string(time_buf) + " " + content;
    // 英文无需编码转换，直接输出
    std::cout << log_content << std::endl;

    // 文件写入仍转GBK（兼容Windows记事本），但内容是英文，无乱码
    std::ofstream log_file(LOG_FILE, std::ios::app);
    if (log_file.is_open()) {
        log_file << utf8_to_gbk(log_content) << std::endl;
        log_file.close();
    }
}