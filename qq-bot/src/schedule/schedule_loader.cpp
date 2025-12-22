#include "schedule_loader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace {
    // 简单 UTF-8 校验（兼容 C++14）
    bool utf8_validate(const std::string& s) {
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(s.data());
        size_t i = 0;
        const size_t len = s.size();
        while (i < len) {
            unsigned char c = bytes[i];
            if ((c & 0x80) == 0) {
                i += 1;
            } else if ((c & 0xE0) == 0xC0) {
                if (i + 1 >= len) return false;
                unsigned char c1 = bytes[i + 1];
                if ((c1 & 0xC0) != 0x80) return false;
                unsigned int cp = ((c & 0x1F) << 6) | (c1 & 0x3F);
                if (cp < 0x80) return false;
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                if (i + 2 >= len) return false;
                unsigned char c1 = bytes[i + 1];
                unsigned char c2 = bytes[i + 2];
                if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) return false;
                unsigned int cp = ((c & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
                if (cp < 0x800) return false;
                if (cp >= 0xD800 && cp <= 0xDFFF) return false;
                i += 3;
            } else if ((c & 0xF8) == 0xF0) {
                if (i + 3 >= len) return false;
                unsigned char c1 = bytes[i + 1];
                unsigned char c2 = bytes[i + 2];
                unsigned char c3 = bytes[i + 3];
                if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return false;
                unsigned int cp = ((c & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
                if (cp < 0x10000) return false;
                if (cp > 0x10FFFF) return false;
                i += 4;
            } else {
                return false;
            }
        }
        return true;
    }
}

bool ScheduleLoader::is_valid_utf8(const std::string& s) {
    return utf8_validate(s);
}

std::string ScheduleLoader::dump_hex(const std::string& s, size_t max_len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    size_t n = s.size();
    if (n > max_len) n = max_len;
    for (size_t i = 0; i < n; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(s[i]));
        if (i + 1 < n) oss << ' ';
    }
    if (s.size() > max_len) {
        oss << " ...";
    }
    return oss.str();
}

std::map<std::string, std::vector<Schedule>> ScheduleLoader::parse_from_json_str(const std::string& json_str) {
    std::map<std::string, std::vector<Schedule>> result;
    try {
        if (json_str.empty()) return result;
        json j = json::parse(json_str);

        const json* data = &j;
        if (j.is_object() && j.contains("data")) {
            data = &j["data"];
        }
        if (data->is_object()) {
            for (auto it = data->begin(); it != data->end(); ++it) {
                if (it.value().is_array()) {
                    result[it.key()] = it.value().get<std::vector<Schedule>>();
                }
            }
        }
    } catch (const std::exception&) {
        // 返回空，交由上层决定如何处理
    }
    return result;
}

bool ScheduleLoader::save_to_file(const std::map<std::string, std::vector<Schedule>>& schedules, const std::string& file_path) {
    json j;
    j["version"] = 1;
    j["data"] = json::object();
    for (const auto& kv : schedules) {
        j["data"][kv.first] = kv.second; // 依赖 Schedule 的 to_json
    }

    std::ofstream ofs(file_path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!ofs.is_open()) return false;
    try {
        const std::string dumped = j.dump(2);
        ofs.write(dumped.data(), static_cast<std::streamsize>(dumped.size()));
        ofs.flush();
        return ofs.good();
    } catch (...) {
        return false;
    }
}

std::map<std::string, std::vector<Schedule>> ScheduleLoader::load_from_file(const std::string& file_path) {
    std::ifstream ifs(file_path, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) return {};

    std::ostringstream ss;
    ss << ifs.rdbuf();
    const std::string content = ss.str();
    return parse_from_json_str(content);
}