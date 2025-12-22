#pragma once
#ifndef CONFIG_H
#define CONFIG_H

// WebSocket 配置（NapCat 地址）
const char* const WS_HOST = "127.0.0.1";
const char* const WS_PORT = "5140";
const char* const WS_PATH = "/ws";

// 机器人配置
const char* const BOT_QQ = "3373368470";  // 替换为你的机器人QQ
const char* const LOG_FILE = "robot_log.txt";  // 日志文件路径

const char* const SCHEDULE_DATA_FILE = "schedules.json";  // 课表数据文件
#endif // CONFIG_H
