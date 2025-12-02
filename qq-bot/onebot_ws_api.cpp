#include "onebot_ws_api.h"
#include "member_cache.h"
#include "utils.h"
#include <mutex>
#include <atomic>

static websocket::stream<tcp_socket>* g_ws = nullptr;
static std::mutex g_ws_write_mtx;

// 生成简单 echo
static std::string make_echo(const std::string& action, const std::string& id)
{
    static std::atomic<unsigned long long> seq{ 1 };
    return action + ":" + id + ":" + std::to_string(seq.fetch_add(1, std::memory_order_relaxed));
}

void onebot_api_init(websocket::stream<tcp_socket>* ws)
{
    g_ws = ws;
}

void onebot_api_fetch_group_member_list(const std::string& group_id)
{
    if (g_ws == nullptr) return;
    nlohmann::json req = {
        {"action", "get_group_member_list"},
        {"params", {
            {"group_id", std::stoll(group_id)}
        }},
        {"echo", make_echo("get_group_member_list", group_id)}
    };
    std::lock_guard<std::mutex> lock(g_ws_write_mtx);
    try {
        g_ws->write(boost::asio::buffer(req.dump()));
        write_log("API sent: get_group_member_list for group " + group_id);
    } catch (const boost::beast::system_error& e) {
        write_log(std::string("API send failed: ") + e.what());
    }
}

void onebot_api_on_frame(const nlohmann::json& frame)
{
    // 仅处理带 echo 的回执
    if (!frame.contains("echo") || !frame["echo"].is_string()) return;
    std::string echo = frame["echo"].get<std::string>();
    if (frame.contains("status") && frame["status"].is_string()) {
        std::string status = frame["status"].get<std::string>();
        if (status != "ok") {
            write_log("API response not ok. echo=" + echo);
            return;
        }
    }

    // 处理 get_group_member_list 回执
    const std::string prefix = "get_group_member_list:";
    auto pos = echo.find(prefix);
    if (pos != std::string::npos) {
        // echo 形如：get_group_member_list:<group_id>:<seq>
        // 解析 group_id
        std::string tail = echo.substr(prefix.size()); // <group_id>:<seq>
        std::string group_id = tail.substr(0, tail.find(':'));

        if (!frame.contains("data") || !frame["data"].is_array()) {
            write_log("API response data missing for member list. echo=" + echo);
            return;
        }
        const auto& arr = frame["data"];
        size_t count = 0;
        for (const auto& item : arr) {
            try {
                if (!item.contains("user_id")) continue;
                std::string qq = std::to_string(item["user_id"].get<long long>());
                std::string name;
                if (item.contains("card") && item["card"].is_string()) {
                    name = trim_space(item["card"].get<std::string>());
                }
                if (name.empty() && item.contains("nickname") && item["nickname"].is_string()) {
                    name = trim_space(item["nickname"].get<std::string>());
                }
                if (name.empty()) name = qq;
                // 写入/更新缓存
                upsert_member_name(group_id, qq, name);
                ++count;
            } catch (...) {
                // ignore single item errors
            }
        }
        write_log("API member list cached. group=" + group_id + ", members=" + std::to_string(count));
        return;
    }
}