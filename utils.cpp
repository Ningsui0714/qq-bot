#include "utils.h"
#include <windows.h>
#include <regex>
#include <string>

// GBK编码转为UTF-8编码
std::string gbk_to_utf8(const std::string& str_gbk) {
    if (str_gbk.empty()) return {};

    // 先按本地ANSI/GBK代码页解码到 UTF-16
    int wlen = MultiByteToWideChar(CP_ACP, 0,
        str_gbk.data(), static_cast<int>(str_gbk.size()),
        nullptr, 0);
    if (wlen <= 0) {
        // 再试指定GBK代码页（936）
        wlen = MultiByteToWideChar(936, 0,
            str_gbk.data(), static_cast<int>(str_gbk.size()),
            nullptr, 0);
        if (wlen <= 0) {
            return str_gbk; // 回退：返回原始数据
        }
        std::wstring wbuf;
        wbuf.resize(wlen);
        int wlen2 = MultiByteToWideChar(936, 0,
            str_gbk.data(), static_cast<int>(str_gbk.size()),
            &wbuf[0], wlen);
        if (wlen2 <= 0) return str_gbk;

        // 再从 UTF-16 编码成 UTF-8
        int u8len = WideCharToMultiByte(CP_UTF8, 0,
            wbuf.data(), wlen2, nullptr, 0, nullptr, nullptr);
        if (u8len <= 0) return str_gbk;
        std::string out;
        out.resize(u8len);
        int u8len2 = WideCharToMultiByte(CP_UTF8, 0,
            wbuf.data(), wlen2, &out[0], u8len, nullptr, nullptr);
        if (u8len2 <= 0) return str_gbk;
        return out;
    }

    std::wstring wbuf;
    wbuf.resize(wlen);
    int wlen2 = MultiByteToWideChar(CP_ACP, 0,
        str_gbk.data(), static_cast<int>(str_gbk.size()),
        &wbuf[0], wlen);
    if (wlen2 <= 0) return str_gbk;

    int u8len = WideCharToMultiByte(CP_UTF8, 0,
        wbuf.data(), wlen2, nullptr, 0, nullptr, nullptr);
    if (u8len <= 0) return str_gbk;
    std::string out;
    out.resize(u8len);
    int u8len2 = WideCharToMultiByte(CP_UTF8, 0,
        wbuf.data(), wlen2, &out[0], u8len, nullptr, nullptr);
    if (u8len2 <= 0) return str_gbk;
    return out;
}

// 移除字符串首尾的空白字符
std::string trim(const std::string& str) {
    if (str.empty()) return {};
    size_t b = 0, e = str.size();

    auto is_space = [](unsigned char c) -> bool {
        // ASCII 空白
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v') return true;
        return false;
    };

    while (b < e && is_space(static_cast<unsigned char>(str[b]))) ++b;
    while (e > b && is_space(static_cast<unsigned char>(str[e - 1]))) --e;

    // 同时去掉常见 UTF-8 的 BOM/NBSP/全角空格等前后缀
    std::string out = str.substr(b, e - b);
    auto strip_prefix = [&](const char* seq, size_t len) {
        if (out.size() >= len && std::memcmp(out.data(), seq, len) == 0) {
            out.erase(0, len);
        }
    };
    auto strip_suffix = [&](const char* seq, size_t len) {
        if (out.size() >= len && std::memcmp(out.data() + out.size() - len, seq, len) == 0) {
            out.erase(out.size() - len, len);
        }
    };
    // U+FEFF BOM, U+00A0 NBSP, U+3000 全角空格
    strip_prefix("\xEF\xBB\xBF", 3);
    strip_prefix("\xC2\xA0", 2);
    strip_prefix("\xE3\x80\x80", 3);
    strip_suffix("\xC2\xA0", 2);
    strip_suffix("\xE3\x80\x80", 3);
    return out;
}

// 与 schedule_set.cpp 保持一致的裁剪函数
std::string trim_space(const std::string& str) {
    return trim(str);
}

// HTML实体反转义
std::string unescape_html_entities(const std::string& str) {
    std::string unescaped = str;
    unescaped = std::regex_replace(unescaped, std::regex("&#91;"), "[");
    unescaped = std::regex_replace(unescaped, std::regex("&#93;"), "]");
    unescaped = std::regex_replace(unescaped, std::regex("&amp;"), "&");
    unescaped = std::regex_replace(unescaped, std::regex("&quot;"), "\"");
    return unescaped;
}