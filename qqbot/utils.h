// utils.h：通用工具函数声明文件
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 函数1：去除字符串所有空格（比如" 1 "变成"1"）
std::string trim_space(const std::string& s);

// 函数2：检查消息是否@机器人
bool is_at_bot(const json& msg_data);

// 函数3：写日志到文件，自动加时间戳
void write_log(const std::string& content);

// 函数4：确保字符串是有效的UTF-8编码（清理无效字节）
std::string ensure_utf8(const std::string& input);

#endif // UTILS_H
