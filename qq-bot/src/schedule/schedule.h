#pragma once
#include "config.h"
#include "utils.h"
#include "msg_handler.h"
#include "reply_generator.h"
#include <vector>
#include <string>

class Schedule {
private:
    int start_week;
    int end_week;
    int start_class;
    int end_class;
    int weekday;
    std::string name;
    std::string qq_number;

public:
    Schedule() = default;

    // 旧构造（兼容），默认 weekday=1
    Schedule(int sw, int ew, int sc, int ec, std::string n, std::string qn)
        : start_week(sw), end_week(ew), start_class(sc), end_class(ec), weekday(1),
          name(std::move(n)), qq_number(std::move(qn)) {
    }

    // 新构造（包含星期几）
    Schedule(int sw, int ew, int sc, int ec, int wd, std::string n, std::string qn)
        : start_week(sw), end_week(ew), start_class(sc), end_class(ec), weekday(wd),
          name(std::move(n)), qq_number(std::move(qn)) {
    }

    // 访问器
    int get_start_week() const { return start_week; }
    int get_end_week() const { return end_week; }
    int get_start_class() const { return start_class; }
    int get_end_class() const { return end_class; }
    int get_weekday() const { return weekday; }
    const std::string& get_name() const { return name; }
    const std::string& get_qq_number() const { return qq_number; }

    friend void to_json(json& j, const Schedule& s) {
        j = json{
            {"start_week", s.start_week},
            {"end_week", s.end_week},
            {"start_class", s.start_class},
            {"end_class", s.end_class},
            {"weekday", s.weekday},
            {"name", s.name},
            {"qq_number", s.qq_number}
        };
    }

    friend void from_json(const json& j, Schedule& s) {
        j.at("start_week").get_to(s.start_week);
        j.at("end_week").get_to(s.end_week);
        j.at("start_class").get_to(s.start_class);
        j.at("end_class").get_to(s.end_class);
        j.at("weekday").get_to(s.weekday);
        j.at("name").get_to(s.name);
        j.at("qq_number").get_to(s.qq_number);
    }

    std::vector<ReplyRule> get_schedule_rules();

    // 使用 UTF-8 字面量
    std::string to_string() const {
        return std::string(u8"课程：") + name +
            std::string(u8"，周次：") + std::to_string(start_week) + "-" + std::to_string(end_week) +
            std::string(u8"，节次：") + std::to_string(start_class) + "-" + std::to_string(end_class) +
            std::string(u8"，星期：") + std::to_string(weekday);
    }
};