#pragma once
#include <string>

// GBK编码转为UTF-8编码
std::string gbk_to_utf8(const std::string& str_gbk);

// 移除字符串首尾的空白字符（ASCII空白 + 常见UTF-8不可见空格）
std::string trim(const std::string& str);

// 切分时使用的轻量空白裁剪（兼容 schedule_set.cpp 的调用）
std::string trim_space(const std::string& str);

// HTML实体反转义
std::string unescape_html_entities(const std::string& str);