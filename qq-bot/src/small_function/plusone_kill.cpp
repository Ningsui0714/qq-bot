#include "plusone_kill.h"
#include <iostream>
#include <regex>

// 每个群的“连续消息”状态：只关心最近一种内容及其连续次数
struct StreakState
{
    std::string last_key;
    int count = 0;
};

// 替换 recent_messages 的定义，类型应与头文件一致
std::unordered_map<std::string, std::unordered_map<std::string, int>>
    PlusOneKill::recent_messages;

// 替换 last_key_map 的定义，类型应与“上一次内容”相关的存储一致
static std::unordered_map<std::string, std::string> last_key_map;

// 从 CQ:image 文本中提取 file=XXX 的值（不含逗号和右括号）
static std::string extract_cq_image_file(const std::string& content)
{
    // content 形如：[CQ:image,summary=[动画表情],file=XXX.png,sub_type=1,...]
    const std::string key = "file=";
    auto pos = content.find(key);
    if (pos == std::string::npos)
    {
        return {};
    }
    pos += key.size();
    // 找到下一个逗号或 ']'
    size_t end = content.find_first_of(",]", pos);
    if (end == std::string::npos)
    {
        end = content.size();
    }
    return content.substr(pos, end - pos);
}

bool PlusOneKill::HandleMessage(const std::string& group_id,
    const std::string& /*user_id*/,
    const std::string& content,
    const json& /*msg_data*/,
    json& reply)
{
    std::string key;

    if (content.compare(0, 9, "[CQ:image") == 0)
    {
        std::string file_key = extract_cq_image_file(content);
        if (file_key.empty())
        {
            return false;
        }
        key = "img:" + file_key;
    }
    else
    {
        if (content.empty())
        {
            return false;
        }
        key = "txt:" + content;
    }

    // 仅在同一个群内统计，不同群互不影响
    int& count = recent_messages[group_id][key];

    // 判断是否与上一次内容相同
    if (last_key_map[group_id] == key)
    {
        ++count;
    }
    else
    {
        last_key_map[group_id] = key;
        count = 1;
    }

    if (count == 3)
    {
        BuildReplyMessage(group_id, reply);
        count = 0;
        last_key_map[group_id].clear();
        return true;
    }

    return false;
}

void PlusOneKill::BuildReplyMessage(const std::string& group_id, json& reply)
{
    // 使用绝对路径测试（确认 OneBot 实现支持绝对路径；否则按实现文档调整）
    std::string image_path = R"(D:\github\qq-bot\qq-bot\plusone_kill.png)";

    reply = json{
        {"action", "send_group_msg"},
        {"params", json{
            {"group_id", group_id},
            {"message", json::array({
                json{
                    {"type", "image"},
                    {"data", json{{"file", image_path}}}
                }
            })}
        }}
    };
}