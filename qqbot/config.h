// config.h：存放全局配置，所有文件都能引用
#ifndef CONFIG_H  // 防止重复引用（C++固定写法，不用懂，照抄就行）
#define CONFIG_H

// 机器人核心配置（修改这里就行，不用动其他文件）
const std::string BOT_QQ = "3373368470";  // 机器人QQ
const std::string WS_HOST = "127.0.0.1";     // NapCat WS地址
const std::string WS_PORT = "3001";          // NapCat WS端口
const std::string LOG_FILE = "robot_log.txt";// 日志保存路径（比如当前目录的robot_log.txt）

#endif // CONFIG_H

