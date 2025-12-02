#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

std::string gbk_to_utf8(const std::string& gbk_str);
std::string utf8_to_gbk(const std::string& utf8_str);
std::string trim_space(const std::string& str);
bool is_at_bot(const json& msg_data);
void write_log(const std::string& content);

// 新增：统一的 @ 封装
inline std::string with_at(const std::string& qq, const std::string& message) {
    return std::string("[CQ:at,qq=") + qq + "] " + message;
}

// 示例/占位，需根据实际上下文完善
nlohmann::json get_current_msg_data();

#endif // UTILS_H