#pragma once
#include "schedule.h"
#include <vector>
#include <string>
#include <map>

class ScheduleLoader {
public:
    static bool is_valid_utf8(const std::string& s);
    static std::string dump_hex(const std::string& s, size_t max_len = 32);
    static std::map<std::string, std::vector<Schedule>> parse_from_json_str(const std::string& json_str);
    static bool save_to_file(const std::map<std::string, std::vector<Schedule>>& schedules, const std::string& file_path);
    static std::map<std::string, std::vector<Schedule>> load_from_file(const std::string& file_path);
};