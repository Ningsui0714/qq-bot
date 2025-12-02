#pragma once
#include "schedule.h"
#include <chrono>
#include <ctime>
#include <string>
#include <vector>

// 作息类型枚举
enum class ScheduleType {
    WINTER, // 冬季作息（10.1-次年5.1）
    SUMMER  // 夏季作息（5.1-10.1）
};

class ScheduleReminder {
public:
    // 设置学期第一周开始日期（格式：YYYY-MM-DD）
    static bool set_term_start_date(const std::string& date_str);

    // 获取指定日期的所有课程（按时间排序）
    static std::vector<Schedule> get_courses_on_date(
        const std::string& qq_number,
        const std::tm& target_date
    );

    // 获取今日课程提醒消息
    static std::string get_today_courses_reminder(const std::string& qq_number);

    // 获取明日课程提醒消息（用于晚10点推送）
    static std::string get_tomorrow_courses_reminder(const std::string& qq_number);

    // 根据时间获取当前作息类型
    static ScheduleType get_schedule_type(const std::tm& date);

private:
    // 学期第一周的起始日期（用于计算当前周数）
    static std::tm term_start_date;

    // 计算指定日期是学期的第几周
    static int get_week_of_term(const std::tm& date);

    // 检查课程是否在指定日期上课
    static bool is_course_on_date(const Schedule& course, const std::tm& date);
};
