#include "config.h"
#include "utils.h"
#include "msg_handler.h"
#include "schedule_reminder.h"
#include "schedule_loader.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <ctime>
#include <Windows.h>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using nlohmann::json;

// 与其他模块保持一致的课表持久化文件路径
static const char* SCHEDULE_FILE = "persistent_schedules.json";

// 占位：根据 qq 映射群号。请替换为真实实现（从配置或数据库加载）。
static std::string get_group_id_by_qq(const std::string& qq) {
    // TODO: 替换为真实映射逻辑
    write_log("get_group_id_by_qq not implemented. QQ=" + qq);
    return std::string(); // 返回空表示未知群
}

// UTF-8 校验：仅检查结构合法性
static bool is_valid_utf8(const std::string& s) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
    size_t i = 0, n = s.size();
    while (i < n) {
        unsigned char c = p[i];
        size_t len = 0;
        if (c <= 0x7F) {
            i += 1;
            continue;
        }
        else if ((c >> 5) == 0x6) {
            len = 2;
        }
        else if ((c >> 4) == 0xE) {
            len = 3;
        }
        else if ((c >> 3) == 0x1E) {
            len = 4;
        }
        else {
            return false;
        }
        if (i + len > n) return false;
        for (size_t k = 1; k < len; ++k) {
            if ((p[i + k] >> 6) != 0x2) return false;
        }
        i += len;
    }
    return true;
}

// 控制字符清理（不破坏多字节 UTF-8）
static std::string strip_control(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        if (c < 32 && c != '\t' && c != '\n' && c != '\r') {
            continue;
        }
        out.push_back(static_cast<char>(c));
    }
    return out;
}

// 十六进制预览
static std::string hex_preview(const std::string& s, size_t max_len = 32) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase;
    size_t limit = std::min(s.size(), max_len);
    for (size_t i = 0; i < limit; ++i) {
        oss << std::setw(2) << std::setfill('0') << (static_cast<unsigned int>(static_cast<unsigned char>(s[i])));
        if (i + 1 != limit) oss << ' ';
    }
    if (s.size() > limit) oss << " ...";
    return oss.str();
}

// 每晚 22:00 推送明日课程提醒
static void reminder_task(websocket::stream<tcp::socket>& ws) {
    while (true) {
        // 当前本地时间
        auto now = std::chrono::system_clock::now();
        std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm;
        localtime_s(&local_tm, &now_tt);

        // 设定目标为今天的 22:00，并规范化
        std::tm target_tm = local_tm;
        target_tm.tm_hour = 22;
        target_tm.tm_min = 0;
        target_tm.tm_sec = 0;
        target_tm.tm_isdst = -1;
        auto target_time = std::chrono::system_clock::from_time_t(std::mktime(&target_tm));

        // 如果当前时间已过今晚 22:00，则推迟到明天 22:00
        if (now > target_time) {
            target_time += std::chrono::hours(24);
        }

        // 等待到目标时间
        std::this_thread::sleep_until(target_time);

        // 发送明日课程提醒给所有有课的用户
        auto all_schedules = ScheduleLoader::load_from_file(SCHEDULE_FILE);
        for (const auto& kv : all_schedules) {
            const std::string& qq = kv.first;
            std::string reminder = ScheduleReminder::get_tomorrow_courses_reminder(qq);
            if (reminder.find(u8"明天没有课程") == std::string::npos) {
                std::string group_id = get_group_id_by_qq(qq); // 请实现真实映射
                if (!group_id.empty()) {
                    json reply = {
                        {"action", "send_group_msg"},
                        {"params", {
                            {"group_id", group_id},
                            {"message", "[CQ:at,qq=" + qq + "] " + reminder}
                        }}
                    };
                    try {
                        ws.write(asio::buffer(reply.dump()));
                    } catch (const beast::system_error& e) {
                        write_log("Reminder send failed: " + std::string(e.what()));
                    }
                }
            }
        }
    }
}

static void run_robot() {
    try {
        asio::io_context ioc;
        tcp::resolver resolver(ioc);
        websocket::stream<tcp::socket> ws(ioc);

        auto results = resolver.resolve(WS_HOST, WS_PORT);
        asio::connect(ws.next_layer(), results.begin(), results.end());
        ws.handshake(WS_HOST, WS_PATH);
        ws.text(true); // 明确文本帧（UTF-8）

        write_log("WebSocket connected successfully! Robot started, QQ: " + std::string(BOT_QQ));
        write_log("Waiting for group messages...");

        // 握手成功后启动提醒线程
        std::thread(reminder_task, std::ref(ws)).detach();

        beast::flat_buffer buffer;
        while (true) {
            buffer.clear();
            ws.read(buffer);
            std::string frame = beast::buffers_to_string(buffer.data());

            auto hex_dump = [](const std::string& s, size_t max_len = 48) {
                std::ostringstream oss;
                oss << std::hex << std::uppercase;
                size_t lim = std::min(s.size(), max_len);
                for (size_t i = 0; i < lim; ++i) {
                    oss << std::setw(2) << std::setfill('0')
                        << (unsigned int)(unsigned char)s[i];
                    if (i + 1 != lim) oss << ' ';
                }
                if (s.size() > lim) oss << " ...";
                return oss.str();
            };

            json msg_data;
            bool ok = false;
            std::string parse_path;

            // 尝试直接解析原始帧
            try {
                msg_data = json::parse(frame, nullptr, true, true);
                ok = true;
                parse_path = "raw-utf8";
            }
            catch (const json::exception& e) {
                if (e.id == 316 || e.id == 101) {
                    write_log("Parse raw failed(id=" + std::to_string(e.id) + "), hex: " + hex_dump(frame));
                    // 回退：尝试按 GBK 解释再转 UTF-8
                    std::string converted = gbk_to_utf8(frame);
                    try {
                        msg_data = json::parse(converted, nullptr, true, true);
                        ok = true;
                        parse_path = "gbk->utf8";
                        write_log("Fallback GBK->UTF8 succeeded, first bytes(hex): " + hex_dump(converted));
                    }
                    catch (const json::exception& e2) {
                        write_log("Fallback parse failed(id=" + std::to_string(e2.id) + "), converted hex: " + hex_dump(converted));
                    }
                }
                else {
                    write_log("Parse raw failed(id=" + std::to_string(e.id) + "): " + std::string(e.what()));
                }
            }

            if (!ok) {
                std::string preview = frame.substr(0, std::min<size_t>(120, frame.size()));
                write_log("Drop frame (unparsed). Preview: " + preview);
                continue;
            }

            // 正常消息路由
            if (msg_data.contains("post_type") && msg_data["post_type"] == "message"
                && msg_data.contains("message_type") && msg_data["message_type"] == "group") {
                write_log("JSON parsed via " + parse_path);
                handle_group_message(msg_data, ws);
            }
            else {
                write_log("Ignore non-group frame");
            }
        }

        ws.close(websocket::close_code::normal);
    }
    catch (const beast::system_error& e) {
        write_log("WebSocket connection failed: " + std::string(e.what()) + ", Error code: " + std::to_string(e.code().value()));
    }
    catch (const std::exception& e) {
        write_log("Program exception: " + std::string(e.what()));
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    run_robot();
    return 0;
}