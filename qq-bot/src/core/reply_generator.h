#pragma once
#ifndef REPLY_GENERATOR_H
#define REPLY_GENERATOR_H

#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <vector>

using json = nlohmann::json;

// 通用规则结构体（外部可自定义规则传入）
struct ReplyRule {
    // 匹配器：判断消息是否符合规则（msg_data=原始消息JSON，content=预处理后文本）
    std::function<bool(const json& msg_data, const std::string& content)> matcher;
    // 回复生成器：生成回复文本（group_id=目标群组ID，content=消息内容）
    std::function<std::string(const std::string& group_id, const std::string& content)> reply_generator;

    // 支持只传 group_id 的重载（兼容旧用法）
    ReplyRule(
        std::function<bool(const json&, const std::string&)> m,
        std::function<std::string(const std::string&)> r)
        : matcher(std::move(m))
        , reply_generator([r](const std::string& group_id, const std::string&) { return r(group_id); })
    {
    }

    // 支持 group_id + content 的新用法
    ReplyRule(
        std::function<bool(const json&, const std::string&)> m,
        std::function<std::string(const std::string&, const std::string&)> r)
        : matcher(std::move(m))
        , reply_generator(std::move(r))
    {
    }

    // 默认构造函数
    ReplyRule() = default;
};

// 通用回复生成函数（支持两种调用方式：用默认规则/自定义规则）
namespace ReplyGenerator {
    // 方式1：使用内置默认规则（兼容原有逻辑）
    bool generate(const json& msg_data, const std::string& content, const std::string& group_id, json& reply);

    // 方式2：传入自定义规则（外部扩展用）
    bool generate_with_rules(const json& msg_data, const std::string& content, const std::string& group_id,
        const std::vector<ReplyRule>& custom_rules, json& reply);
}

#endif // REPLY_GENERATOR_H