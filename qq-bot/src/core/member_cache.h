#pragma once
#include "utils.h"
#include <string>

// 初始化（从文件加载）
void init_member_cache();

// 在收到群消息时更新缓存（优先 card，其次 nickname）
void update_member_display_name(const json& msg_data);

// 获取显示名（优先缓存的群名片/昵称，取不到则返回 qq）
std::string get_display_name(const std::string& group_id, const std::string& qq);

// 新增：写入/更新单个成员名片（供主动拉取时调用）
void upsert_member_name(const std::string& group_id, const std::string& qq, const std::string& name);

// 新增：获取当前已缓存的群成员QQ列表（基于最近发言记录）
std::vector<std::string> get_group_member_qqs(const std::string& group_id);