#include "schedule_loader.h"
#include "utils.h"
#include <vector>
#include <fstream>
#include <string>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 简单 UTF-8 校验（线性扫描）
bool ScheduleLoader::is_valid_utf8(const std::string& s) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(s.data());
    size_t i = 0;
    size_t len = s.size();
    while (i < len) {
        unsigned char c = bytes[i];
        if (c <= 0x7F) { // ASCII
            i++;
        }
        else if ((c >> 5) == 0x6) { // 2 字节
            if (i + 1 >= len) return false;
            if ((bytes[i + 1] >> 6) != 0x2) return false;
            i += 2;
        }
        else if ((c >> 4) == 0xE) { // 3 字节
            if (i + 2 >= len) return false;
            if ((bytes[i + 1] >> 6) != 0x2 || (bytes[i + 2] >> 6) != 0x2) return false;
            i += 3;
        }
        else if ((c >> 3) == 0x1E) { // 4 字节
            if (i + 3 >= len) return false;
            if ((bytes[i + 1] >> 6) != 0x2 || (bytes[i + 2] >> 6) != 0x2 || (bytes[i + 3] >> 6) != 0x2)
                return false;
            i += 4;
        }
        else {
            return false;
        }
    }
    return true;
}

// 记录原始失败的十六进制（便于调试）
std::string ScheduleLoader::dump_hex(const std::string& s, size_t max_len) {
    std::string out;
    char buf[8];
    size_t limit = s.size() < max_len ? s.size() : max_len;
    for (size_t i = 0; i < limit; ++i) {
        std::snprintf(buf, sizeof(buf), "%02X ", static_cast<unsigned char>(s[i]));
        out += buf;
    }
    if (limit < s.size()) out += "...";
    return out;
}

// 从JSON字符串解析课表（自动编码处理）
std::vector<Schedule> ScheduleLoader::parse_from_json_str(const std::string& json_str) {
    std::string work = json_str;
    // 先尝试直接 UTF-8 解析
    try {
        if (!is_valid_utf8(work)) {
            // 非法 UTF-8 → 尝试按 GBK 转换
            std::string converted = gbk_to_utf8(work);
            if (is_valid_utf8(converted)) {
                work = converted;
            }
        }
        json j = json::parse(work);
        std::vector<Schedule> schedules;
        j.at("schedules").get_to(schedules);
        return schedules;
    }
    catch (const json::exception& e) {
        // 捕获 JSON 异常后再尝试一次 GBK → UTF-8 回退（防止边缘情况）
        try {
            std::string converted = gbk_to_utf8(work);
            if (converted != work && is_valid_utf8(converted)) {
                json j2 = json::parse(converted);
                std::vector<Schedule> schedules2;
                j2.at("schedules").get_to(schedules2);
                write_log("ScheduleLoader: fallback GBK->UTF8 success.");
                return schedules2;
            }
        }
        catch (...) {
            // 忽略二次异常
        }
        write_log("ScheduleLoader parse failed. Hex head: " + dump_hex(work));
        throw std::runtime_error("JSON解析失败：" + std::string(e.what()));
    }
    catch (...) {
        write_log("ScheduleLoader unknown parse error. Hex head: " + dump_hex(work));
        throw std::runtime_error("未知解析错误");
    }
}

// 保存课表到JSON文件（vector<Schedule> → JSON文件）
bool ScheduleLoader::save_to_file(const std::vector<Schedule>& schedules, const std::string& file_path) {
    try {
        json j;
        j["schedules"] = schedules; // 序列化课表数组
        std::ofstream file(file_path, std::ios::trunc);
        if (!file.is_open()) {
            write_log("Failed to open file for writing: " + file_path);
            return false;
        }
        file << std::setw(4) << j; // 格式化输出，便于阅读
        file.close();
        return true;
    }
    catch (const std::exception& e) {
        write_log("Save schedule failed: " + std::string(e.what()));
        return false;
    }
    catch (...) {
        write_log("Unknown error when saving schedule");
        return false;
    }
}

// 从文件加载课表（JSON文件 → vector<Schedule>）
std::vector<Schedule> ScheduleLoader::load_from_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        // 文件不存在时返回空列表（首次启动场景）
        write_log("Schedule file not found: " + file_path + ", return empty list");
        return {};
    }
    try {
        json j;
        file >> j;
        file.close();
        std::vector<Schedule> schedules;
        j.at("schedules").get_to(schedules);
        return schedules;
    }
    catch (const std::exception& e) {
        file.close();
        write_log("Load schedule failed: " + std::string(e.what()));
        throw std::runtime_error("文件解析失败");
    }
}