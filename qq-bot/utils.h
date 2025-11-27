#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 1. 编码转换：GBK → UTF-8（Windows 原生 API，稳定无乱码）
std::string gbk_to_utf8(const std::string& gbk_str);

// 2. 编码转换：UTF-8 → GBK（日志写入时用）
std::string utf8_to_gbk(const std::string& utf8_str);

// 3. 去除字符串首尾空格
std::string trim_space(const std::string& str);

// 4. 判断消息是否@机器人
bool is_at_bot(const json& msg_data);

// 5. 写日志（支持中文，自动处理编码）
void write_log(const std::string& content);

#endif // UTILS_H
