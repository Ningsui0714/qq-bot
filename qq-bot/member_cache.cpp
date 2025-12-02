#include "member_cache.h"
#include <map>
#include <fstream>

static const char* MEMBER_NAME_FILE = "group_member_names.json";

// group_id -> (qq -> name)
static std::map<std::string, std::map<std::string, std::string>> g_names;

static void load_from_file()
{
    g_names.clear();
    std::ifstream ifs(MEMBER_NAME_FILE, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) return;

    try {
        nlohmann::json j;
        ifs >> j;
        if (j.is_object()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                const std::string group_id = it.key();
                if (!it.value().is_object()) continue;
                for (auto it2 = it.value().begin(); it2 != it.value().end(); ++it2) {
                    const std::string qq = it2.key();
                    const std::string name = it2.value().is_string() ? it2.value().get<std::string>() : "";
                    if (!name.empty()) {
                        g_names[group_id][qq] = name;
                    }
                }
            }
        }
    } catch (...) {
        // ignore
    }
}

static void save_to_file()
{
    nlohmann::json j = nlohmann::json::object();
    for (const auto& g : g_names) {
        nlohmann::json inner = nlohmann::json::object();
        for (const auto& kv : g.second) {
            inner[kv.first] = kv.second;
        }
        j[g.first] = inner;
    }
    std::ofstream ofs(MEMBER_NAME_FILE, std::ios::out | std::ios::trunc | std::ios::binary);
    if (ofs.is_open()) {
        ofs << j.dump();
    }
}

void init_member_cache()
{
    load_from_file();
    write_log("Member cache initialized, groups: " + std::to_string(g_names.size()));
}

void update_member_display_name(const json& msg_data)
{
    try {
        if (!msg_data.contains("group_id") || !msg_data["group_id"].is_number()) return;
        std::string group_id = std::to_string(msg_data["group_id"].get<long long>());

        std::string qq;
        if (msg_data.contains("user_id") && msg_data["user_id"].is_number()) {
            qq = std::to_string(msg_data["user_id"].get<long long>());
        } else if (msg_data.contains("sender") && msg_data["sender"].is_object()
                   && msg_data["sender"].contains("user_id") && msg_data["sender"]["user_id"].is_number()) {
            qq = std::to_string(msg_data["sender"]["user_id"].get<long long>());
        } else {
            return;
        }

        std::string name;
        if (msg_data.contains("sender") && msg_data["sender"].is_object()) {
            const auto& s = msg_data["sender"];
            if (s.contains("card") && s["card"].is_string()) {
                name = trim_space(s["card"].get<std::string>());
            }
            if (name.empty() && s.contains("nickname") && s["nickname"].is_string()) {
                name = trim_space(s["nickname"].get<std::string>());
            }
        }
        if (name.empty()) name = qq;

        auto& by_group = g_names[group_id];
        auto it = by_group.find(qq);
        if (it == by_group.end() || it->second != name) {
            by_group[qq] = name;
            save_to_file();
        }
    } catch (...) {
        // ignore
    }
}

std::string get_display_name(const std::string& group_id, const std::string& qq)
{
    auto g_it = g_names.find(group_id);
    if (g_it == g_names.end()) return qq;
    auto u_it = g_it->second.find(qq);
    if (u_it == g_it->second.end() || u_it->second.empty()) return qq;
    return u_it->second;
}

void upsert_member_name(const std::string& group_id, const std::string& qq, const std::string& name)
{
    auto& by_group = g_names[group_id];
    auto it = by_group.find(qq);
    if (it == by_group.end() || it->second != name) {
        by_group[qq] = name.empty() ? qq : name;
        save_to_file();
    }
}

// 新增：返回当前缓存中该群的成员QQ列表
std::vector<std::string> get_group_member_qqs(const std::string& group_id)
{
    std::vector<std::string> result;
    auto it = g_names.find(group_id);
    if (it == g_names.end()) return result;
    result.reserve(it->second.size());
    for (const auto& kv : it->second) {
        result.push_back(kv.first);
    }
    return result;
}