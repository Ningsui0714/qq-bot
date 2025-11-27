#pragma once
#pragma once
#include "schedule.h"
#include "utils.h"
#include <vector>
#include <fstream>
#include <string>
#include <stdexcept>

class ScheduleLoader {
private:
    // 简单 UTF-8 校验（线性扫描）
    static bool is_valid_utf8(const std::string& s);
    // 记录原始失败的十六进制（便于调试）
    static std::string dump_hex(const std::string& s, size_t max_len = 128);
public:
    // 从JSON字符串解析课表（自动编码处理）
    static std::vector<Schedule> parse_from_json_str(const std::string& json_str);
    // 保存课表到JSON文件（vector<Schedule> → JSON文件）
    static bool save_to_file(const std::vector<Schedule>& schedules, const std::string& file_path = "schedule.json");
    // 从文件加载课表（JSON文件 → vector<Schedule>）
    static std::vector<Schedule> load_from_file(const std::string& file_path = "schedule.json");
};