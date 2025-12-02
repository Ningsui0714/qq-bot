#pragma once
#include <string>
#include <vector>
#include <set>

// 初始化（加载持久化文件）
bool init_group_mapping();

// 获取用户绑定的群号，若无返回空
std::string get_group_id_by_qq(const std::string& qq);

// 设置用户绑定的群号（覆盖写入并持久化）
void set_group_id_for_qq(const std::string& qq, const std::string& group_id);

// 查询群相关
void add_query_group(const std::string& group_id);
void remove_query_group(const std::string& group_id);
std::set<std::string> get_query_groups();

// 提醒群相关
void set_reminder_group(const std::string& group_id);
std::string get_reminder_group();
void clear_reminder_group();
