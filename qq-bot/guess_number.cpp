#include "reply_generator.h"
#include "config.h"
#include "utils.h"
#include <random>
#include <ctime>
#include <vector>
#include <unordered_map>

// 记录每个群的目标数字
static std::unordered_map<std::string, int> group_target_num;
// 记录每个群当前可猜的下界与上界（包含）
static std::unordered_map<std::string, int> group_low_bound;
static std::unordered_map<std::string, int> group_high_bound;

// 初始化随机数生成器
static std::mt19937 init_rng() {
    std::random_device rd;
    return std::mt19937(rd());
}
static std::mt19937 rng = init_rng();

// 生成1-100随机数
static int generate_random_num() {
    std::uniform_int_distribution<> dist(1, 100);
    return dist(rng);
}

std::vector<ReplyRule> get_guess_number_rules() {
    std::vector<ReplyRule> rules;

    // 规则1：@机器人 + "猜数" → 启动游戏
    rules.push_back(ReplyRule{
        [](const json& msg_data, const std::string& content) {
            return is_at_bot(msg_data) && content == u8"猜数";
        },
        [](const std::string& group_id) {
            int target = generate_random_num();
            group_target_num[group_id] = target; // 记录目标数字
            group_low_bound[group_id] = 1;
            group_high_bound[group_id] = 100;
            return u8"猜数游戏开始！我已生成 1-100 之间的数字，当前范围：[1,100]，请输入你的猜测～\n游戏过程中不需要@bot，退出游戏需要@bot";
        }
        });

    // 规则2：游戏启动后 + 发送数字 → 判断大小并缩小范围
    rules.push_back(ReplyRule{
        [](const json& msg_data, const std::string& content) {
            std::string group_id = std::to_string(msg_data["group_id"].get<long long>());
            if (group_target_num.find(group_id) == group_target_num.end()) {
                return false;
            }
            try {
                std::stoi(content);
                return true;
            }
 catch (...) {
  return false;
}
},
[](const std::string& group_id, const std::string& content) {
    int user_guess;
    try {
        user_guess = std::stoi(content);
    }
catch (...) {
 return std::string(u8"请输入有效数字～");
}

// 当前有效范围
int low = group_low_bound[group_id];
int high = group_high_bound[group_id];

// 检查是否在范围内
if (user_guess < low || user_guess > high) {
    return std::string(u8"当前有效范围是 [") + std::to_string(low) + "," + std::to_string(high) +
        u8"]，请输入范围内的数字～";
}

int target = group_target_num[group_id];

if (user_guess > target) {
    // 缩小上界
    group_high_bound[group_id] = user_guess - 1;
    return std::string(u8"太大啦！新的范围：[") +
        std::to_string(group_low_bound[group_id]) + "," +
        std::to_string(group_high_bound[group_id]) + u8"]";
}
else if (user_guess < target) {
    // 缩小下界
    group_low_bound[group_id] = user_guess + 1;
    return std::string(u8"太小啦！新的范围：[") +
        std::to_string(group_low_bound[group_id]) + "," +
        std::to_string(group_high_bound[group_id]) + u8"]";
}
else {
    // 猜中 → 清理数据
    group_target_num.erase(group_id);
    group_low_bound.erase(group_id);
    group_high_bound.erase(group_id);
    return std::string(u8"恭喜猜对啦！🎉 就是 ") + std::to_string(target) +
        u8" ～ 输入 '猜数' 可重新开始游戏";
}
}
        });

    // 规则3：游戏启动后 + 发送"退出" → 结束游戏
    rules.push_back(ReplyRule{
        [](const json& msg_data, const std::string& content) {
            std::string group_id = std::to_string(msg_data["group_id"].get<long long>());
            return is_at_bot(msg_data) && content == u8"退出" && group_target_num.count(group_id) > 0;
        },
        [](const std::string& group_id) {
            group_target_num.erase(group_id);
            group_low_bound.erase(group_id);
            group_high_bound.erase(group_id);
            return u8"猜数游戏已退出～ 输入'猜数'可重新开始";
        }
        });

    return rules;
}