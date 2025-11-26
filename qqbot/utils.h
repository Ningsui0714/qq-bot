// utils.h：声明通用工具函数，供其他文件调用
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <nlohmann/json.hpp>  // 用到JSON，要包含头文件
using json = nlohmann::json;

// 工具1：去除字符串所有空格（比如“ 1 ”→“1”）
std::string trim_space(const std::string& s);

// 工具2：检测消息是否@机器人（参数是NapCat发来的JSON消息）
bool is_at_bot(const json& msg_data);

// 工具3：写日志到文件（参数是日志内容，自动加时间戳）
void write_log(const std::string& content);

//gbk编码转换为utf8编码
std::string gbk_to_utf8(const std::string& gbk_str);


#endif // UTILS_H#pragma once
