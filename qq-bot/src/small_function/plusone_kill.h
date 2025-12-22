#ifndef PLUSONE_KILL_H
#define PLUSONE_KILL_H

#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class PlusOneKill {
public:
    // 外部调用入口：处理每条群消息，返回是否需要回复
    static bool HandleMessage(const std::string& group_id,
        const std::string& user_id,
        const std::string& content,
        const json& msg_data,
        json& reply);

private:
    // 构造回复消息（发送本地图片）
    static void BuildReplyMessage(const std::string& group_id, json& reply);

    // 按群 + 消息内容统计出现次数
    static std::unordered_map<std::string,
        std::unordered_map<std::string, int>> recent_messages;
};

#endif // PLUSONE_KILL_H




