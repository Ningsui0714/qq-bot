#include "group_mapping.h"
#include "utils.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <map>
#include <mutex>

using json = nlohmann::json;

// 持久化文件
static const char* GROUP_MAPPING_FILE = "group_mapping.json";

// 内存数据
static std::map<std::string, std::string> s_user_group; // qq -> group_id
static std::set<std::string> s_query_groups;
static std::string s_reminder_group;

// 线程安全
static std::mutex s_mtx;

static bool save_all_locked()
{
    json j;
    j["bindings"] = json::object();
    for (const auto& kv : s_user_group)
    {
        j["bindings"][kv.first] = kv.second;
    }

    j["query_groups"] = json::array();
    for (const auto& g : s_query_groups)
    {
        j["query_groups"].push_back(g);
    }

    j["reminder_group"] = s_reminder_group;

    try
    {
        std::ofstream ofs(GROUP_MAPPING_FILE, std::ios::binary | std::ios::trunc);
        if (!ofs.is_open())
        {
            write_log("Open group_mapping.json failed for write");
            return false;
        }
        ofs << j.dump(2);
        ofs.close();
        return true;
    }
    catch (const std::exception& e)
    {
        write_log(std::string("Save group mapping failed: ") + e.what());
        return false;
    }
}

static bool load_all_locked()
{
    s_user_group.clear();
    s_query_groups.clear();
    s_reminder_group.clear();

    std::ifstream ifs(GROUP_MAPPING_FILE, std::ios::binary);
    if (!ifs.is_open())
    {
        // 首次运行不存在不视为错误
        write_log("group_mapping.json not found, start with empty mappings");
        return true;
    }

    try
    {
        json j;
        ifs >> j;

        if (j.contains("bindings") && j["bindings"].is_object())
        {
            for (auto it = j["bindings"].begin(); it != j["bindings"].end(); ++it)
            {
                if (it.value().is_string())
                {
                    s_user_group[it.key()] = it.value().get<std::string>();
                }
            }
        }

        if (j.contains("query_groups") && j["query_groups"].is_array())
        {
            for (const auto& v : j["query_groups"])
            {
                if (v.is_string())
                {
                    s_query_groups.insert(v.get<std::string>());
                }
            }
        }

        if (j.contains("reminder_group") && j["reminder_group"].is_string())
        {
            s_reminder_group = j["reminder_group"].get<std::string>();
        }
        return true;
    }
    catch (const std::exception& e)
    {
        write_log(std::string("Load group mapping failed: ") + e.what());
        return false;
    }
}

// 初始化（加载持久化文件）
bool init_group_mapping()
{
    std::lock_guard<std::mutex> _guard(s_mtx);
    bool ok = load_all_locked();
    if (ok)
    {
        write_log("Group mapping loaded: "
            + std::to_string(s_user_group.size()) + " bindings, "
            + std::to_string(s_query_groups.size()) + " query groups, reminder="
            + (s_reminder_group.empty() ? std::string("<empty>") : s_reminder_group));
    }
    return ok;
}

// 获取用户绑定的群号，若无返回空
std::string get_group_id_by_qq(const std::string& qq)
{
    std::lock_guard<std::mutex> _guard(s_mtx);
    auto it = s_user_group.find(qq);
    return it == s_user_group.end() ? std::string() : it->second;
}

// 设置用户绑定的群号（覆盖写入并持久化）
void set_group_id_for_qq(const std::string& qq, const std::string& group_id)
{
    std::lock_guard<std::mutex> _guard(s_mtx);
    s_user_group[qq] = group_id;
    if (!save_all_locked())
    {
        write_log("Persist set_group_id_for_qq failed");
    }
}

// 查询群相关
void add_query_group(const std::string& group_id)
{
    std::lock_guard<std::mutex> _guard(s_mtx);
    s_query_groups.insert(group_id);
    if (!save_all_locked())
    {
        write_log("Persist add_query_group failed");
    }
}

void remove_query_group(const std::string& group_id)
{
    std::lock_guard<std::mutex> _guard(s_mtx);
    s_query_groups.erase(group_id);
    if (!save_all_locked())
    {
        write_log("Persist remove_query_group failed");
    }
}

std::set<std::string> get_query_groups()
{
    std::lock_guard<std::mutex> _guard(s_mtx);
    return s_query_groups; // 返回拷贝
}

// 提醒群相关
void set_reminder_group(const std::string& group_id)
{
    std::lock_guard<std::mutex> _guard(s_mtx);
    s_reminder_group = group_id;
    if (!save_all_locked())
    {
        write_log("Persist set_reminder_group failed");
    }
}

std::string get_reminder_group()
{
    std::lock_guard<std::mutex> _guard(s_mtx);
    return s_reminder_group;
}

void clear_reminder_group()
{
    std::lock_guard<std::mutex> _guard(s_mtx);
    s_reminder_group.clear();
    if (!save_all_locked())
    {
        write_log("Persist clear_reminder_group failed");
    }
}