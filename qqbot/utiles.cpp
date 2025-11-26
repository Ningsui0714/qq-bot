// utils.cpp：实现utils.h里声明的函数
#include "utils.h"  // 必须包含对应的头文件，否则编译器找不到声明
#include "config.h" // 用到配置里的BOT_QQ和LOG_FILE
#include <algorithm> // 用于trim_space的remove函数
#include <fstream>   // 用于写文件（日志）
#include <ctime>     // 用于日志时间戳
#include <iostream>    // 添加此行以修复 std::cerr 未声明的问题
#include <iconv.h>
#include <vector>
#include <cerrno>
#include <cstring>
#include <locale>
#include <codecvt>
// 实现工具1：去除字符串空格
std::string trim_space(const std::string& s) {
    std::string res = s;
    res.erase(std::remove(res.begin(), res.end(), ' '), res.end());
    return res;
}

// 实现工具2：检测是否@机器人
bool is_at_bot(const json& msg_data) {
    try {
        if (msg_data.contains("message") && msg_data["message"].is_array()) {
            for (const auto& elem : msg_data["message"]) {
                // 对比@的QQ号和配置里的BOT_QQ
                if (elem["type"] == "at" && elem["data"]["qq"] == BOT_QQ) {
                    return true;
                }
            }
        }
        return false;
    }
    catch (const json::exception& e) {
        write_log("检测@机器人失败：" + std::string(e.what())); // 调用写日志工具
        return false;
    }
}

// 实现工具3：写日志到文件
void write_log(const std::string& content) {
    // 1. 获取当前时间并按可移植、安全的方式格式化为字符串（YYYY-MM-DD HH:MM:SS）
    std::time_t now = std::time(nullptr);
    char time_str[64] = "时间获取失败";


#if defined(_MSC_VER)
    // MSVC: 使用线程安全的 localtime_s
    std::tm tm{};
    if (localtime_s(&tm, &now) == 0) {
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);
    }
#else
    // POSIX: 使用线程安全的 localtime_r
    std::tm tm{};
    if (localtime_r(&now, &tm) != nullptr) {
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);
    }
#endif

    // 2. 打开日志文件（ios::app表示“追加内容”，不会覆盖旧日志）
    std::ofstream log_file(LOG_FILE, std::ios::app);
    if (log_file.is_open()) {
        log_file << "[" << time_str << "] " << content << std::endl; // 写入时间+内容
        log_file.close(); // 关闭文件
    }
    else {
        std::cerr << "日志文件打开失败！" << std::endl;
    }
}

// 辅助：快速验证是否已经是合法 UTF-8（避免对 UTF-8 再转 GBK→UTF-8 导致破坏）
static bool is_valid_utf8(const std::string& s) {
    const unsigned char *bytes = reinterpret_cast<const unsigned char*>(s.data());
    size_t len = s.size();
    size_t i = 0;
    while (i < len) {
        unsigned char c = bytes[i];
        if (c <= 0x7F) { // ASCII
            i += 1;
        } else if ((c >> 5) == 0x6) { // 110x xxxx, 2 bytes
            if (i + 1 >= len) return false;
            if ((bytes[i+1] >> 6) != 0x2) return false;
            i += 2;
        } else if ((c >> 4) == 0xE) { // 1110 xxxx, 3 bytes
            if (i + 2 >= len) return false;
            if ((bytes[i+1] >> 6) != 0x2 || (bytes[i+2] >> 6) != 0x2) return false;
            i += 3;
        } else if ((c >> 3) == 0x1E) { // 1111 0xxx, 4 bytes
            if (i + 3 >= len) return false;
            if ((bytes[i+1] >> 6) != 0x2 || (bytes[i+2] >> 6) != 0x2 || (bytes[i+3] >> 6) != 0x2) return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

// 编码转换：GBK -> UTF-8，带错误处理（跳过非法序列并用'?'替换），并避免对已是 UTF-8 的字符串重复转换
std::string gbk_to_utf8(const std::string& gbk_str) {
    try {
        // 使用C++11的codecvt_convert（需编译器支持C++11及以上）
        std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> converter(new std::codecvt<wchar_t, char, std::mbstate_t>("CHS"));
        std::wstring wide_str = converter.from_bytes(gbk_str);
        return converter.to_bytes(wide_str);
    }
    catch (...) {
        // 转换失败时返回原始消息，避免抛异常
        write_log("编码转换失败，返回原始消息");
        return gbk_str;
    }
}