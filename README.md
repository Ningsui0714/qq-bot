# QQ Bot 🤖

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)](https://www.microsoft.com/windows)

一个功能丰富的 QQ 群聊智能机器人，基于 C++17 和 WebSocket 协议开发，专为学生群体设计，提供课表管理、上课查询、课程提醒、小游戏等实用功能。

## ✨ 功能特性

### 📚 课表管理系统
- **智能导入**：支持单行或多行批量导入课程信息，支持中文逗号和分号分隔
- **灵活查询**：随时查询个人课表，支持按日期查询
- **今日课程**：快速查看今天的所有课程安排
- **课表清空**：一键清空所有课程数据

### 🔍 上课查询功能
- **群内查询**：实时统计群内成员的上课状态
- **绑定管理**：支持绑定/取消绑定群聊，启用或关闭上课查询功能
- **成员缓存**：自动缓存群成员信息，提升查询效率

### ⏰ 智能提醒系统
- **个人提醒**：每晚 22:00 推送个人明日课程提醒
- **群组提醒**：支持全局唯一提醒群，推送全体成员的明日课程
- **提醒绑定**：灵活设置和取消个人/群组提醒功能
- **作息适配**：自动适配冬季/夏季作息时间（10月-5月 / 5月-10月）

### 🎮 趣味小游戏
- **猜数游戏**：经典的 1-100 猜数字游戏，支持多群独立游戏状态
- **+1 击杀**：检测群内重复消息，达到阈值时发送击杀图片

### 💬 自动回复与交互
- **关键词匹配**：支持自定义关键词自动回复
- **帮助系统**：完整的功能说明和使用指南
- **友好交互**：支持中英文问候和测试命令

### 🛠️ 系统特性
- **日志记录**：完善的日志系统，记录所有操作和异常信息
- **数据持久化**：课表、群组映射、成员信息自动保存和加载
- **异常处理**：完善的异常捕获和错误处理机制
- **UTF-8 支持**：全面支持中文字符处理

## 🎯 技术栈

| 技术 | 说明 |
|------|------|
| **语言** | C++17 |
| **网络框架** | Boost.Asio + Boost.Beast（WebSocket） |
| **JSON 解析** | nlohmann/json |
| **QQ 协议** | OneBot v11（通过 NapCat 实现） |
| **编译环境** | Visual Studio 2019/2022 |
| **平台** | Windows x64 |

## 📦 依赖项

- [Boost](https://www.boost.org/) (>= 1.70) - 用于网络通信和异步 I/O
- [nlohmann/json](https://github.com/nlohmann/json) (>= 3.11) - JSON 序列化和反序列化
- [NapCat](https://github.com/NapNeko/NapCatQQ) - QQ 协议实现和 WebSocket 服务

## 📂 项目结构

```
qq-bot/
├── qq-bot/                           # 源代码目录
│   ├── src/                          # 源代码
│   │   ├── core/                     # 核心模块
│   │   │   ├── msg_handler.cpp/h    # 群消息处理器
│   │   │   ├── reply_generator.cpp/h # 回复生成器（规则引擎）
│   │   │   ├── member_cache.cpp/h    # 成员信息缓存
│   │   │   └── group_mapping.cpp/h   # 群组映射管理
│   │   ├── schedule/                 # 课表模块
│   │   │   ├── schedule.h            # 课表数据结构
│   │   │   ├── schedule_set.cpp      # 课表规则集
│   │   │   ├── schedule_loader.cpp/h # 课表持久化
│   │   │   ├── schedule_reminder.cpp/h # 课程提醒
│   │   │   └── class_inquiry.cpp/h   # 上课查询
│   │   ├── small_function/           # 小功能模块
│   │   │   ├── guess_number.cpp/h    # 猜数游戏
│   │   │   └── plusone_kill.cpp/h    # +1击杀功能
│   │   ├── utils/                    # 工具类
│   │   │   └── utils.cpp/h           # 工具函数（编码、日志等）
│   │   ├── main.cpp                  # 程序入口
│   │   ├── config.h                  # 配置文件
│   │   └── onebot_ws_api.cpp/h       # OneBot API 封装
│   ├── qq-bot.sln                    # Visual Studio 解决方案
│   └── qq-bot.vcxproj                # Visual Studio 项目文件
├── LICENSE                            # MIT 许可证
├── .gitignore                         # Git 忽略配置
└── README.md                          # 项目说明文档
```

### 运行时生成的文件

| 文件 | 说明 |
|------|------|
| `robot_log.txt` | 机器人日志文件 |
| `persistent_schedules.json` | 课表持久化数据 |
| `group_mapping.json` | 群组映射关系 |
| `group_member_names.json` | 群成员信息缓存 |
| `term_start_date.txt` | 学期开始日期 |

> **注意**：上述运行时文件已添加到 `.gitignore`，不会提交到仓库。

## ⚙️ 配置说明

### 1. 编辑配置文件

编辑 `qq-bot/src/config.h` 文件：

```cpp
// WebSocket 配置（NapCat 地址和端口）
const char* const WS_HOST = "127.0.0.1";     // WebSocket 服务器地址
const char* const WS_PORT = "5140";           // WebSocket 服务器端口
const char* const WS_PATH = "/ws";            // WebSocket 路径

// 机器人配置
const char* const BOT_QQ = "你的机器人QQ号";  // 替换为你的机器人QQ
const char* const LOG_FILE = "robot_log.txt"; // 日志文件路径
```

### 2. 配置 NapCat

确保 NapCat 已正确安装和配置，并开启 WebSocket 服务：

```json
{
  "ws": {
    "enable": true,
    "host": "127.0.0.1",
    "port": 5140
  }
}
```

## 🚀 编译与运行

### 前置条件

1. **操作系统**：Windows 10/11 (x64)
2. **编译器**：Visual Studio 2019 或更高版本（需包含 C++ 桌面开发工作负载）
3. **包管理器**：[vcpkg](https://github.com/microsoft/vcpkg)

### 安装依赖

使用 vcpkg 安装必要的库：

```bash
# 安装 vcpkg（如果尚未安装）
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 安装依赖库
.\vcpkg install boost:x64-windows
.\vcpkg install nlohmann-json:x64-windows

# 集成 vcpkg 到 Visual Studio
.\vcpkg integrate install
```

### 编译步骤

1. **克隆仓库**
   ```bash
   git clone https://github.com/Ningsui0714/qq-bot.git
   cd qq-bot
   ```

2. **打开解决方案**
   - 使用 Visual Studio 打开 `qq-bot/qq-bot.sln`

3. **配置项目**
   - 确保 vcpkg 已正确集成
   - 选择 **Release** 配置和 **x64** 平台

4. **编译项目**
   - 点击 **生成 > 生成解决方案** 或按 `Ctrl+Shift+B`

5. **查看输出**
   - 编译成功后，可执行文件位于 `qq-bot/x64/Release/qq-bot.exe`

### 运行机器人

1. **启动 NapCat**
   - 确保 NapCat 已启动并正确登录 QQ 账号
   - 确认 WebSocket 服务已开启

2. **运行机器人**
   ```bash
   cd qq-bot/x64/Release
   ./qq-bot.exe
   ```

3. **验证连接**
   - 查看控制台输出，确认 WebSocket 连接成功
   - 检查 `robot_log.txt` 日志文件

## 📖 使用说明

### 功能总览

在群聊中 @机器人 发送 `帮助`、`功能` 或 `指令` 可查看完整功能列表。

### 一、课表管理

#### 导入课表

**方式 1：查看导入格式**
```
@机器人 导入课表
```
机器人会返回导入格式说明。

**方式 2：单行导入**
```
高等数学,1,1,16,1,2
```
格式：`课程名,星期,开始周,结束周,开始节,结束节`

参数说明：
- **课程名**：课程名称（支持中文）
- **星期**：1-7（1=周一，7=周日）
- **开始周**：课程开始的周数（1-20）
- **结束周**：课程结束的周数（1-20）
- **开始节**：课程开始的节次（1-12）
- **结束节**：课程结束的节次（1-12）

**方式 3：批量导入**
```
高等数学,1,1,16,1,2
大学英语,2,1,16,3,4；
线性代数,3,1,16,1,2
```
支持换行或中文分号 `；` 分隔多个课程。

#### 查询课表

```
@机器人 查询课表
```
查看你已导入的所有课程。

#### 今日课程

```
@机器人 今日课程
```
查看今天的课程安排（自动根据当前周数和星期筛选）。

#### 清空课表

```
@机器人 清空课表
```
清空你的所有课程数据（需确认）。

### 二、上课查询

#### 查询谁在上课

```
@机器人 有谁在上课
```
统计当前时间段内，群内哪些成员正在上课。

#### 绑定群聊

```
绑定群聊
```
将本群启用"上课查询"功能。

#### 取消绑定群聊

```
取消绑定群聊
```
关闭本群的"上课查询"功能。

### 三、提醒功能

#### 设置个人提醒

```
设置提醒群
```
将"你的个人提醒"绑定到本群，每晚 22:00 推送你的明日课程。

#### 设置群组提醒

```
绑定群提醒
```
将本群设置为"全局唯一提醒群"，每晚 22:00 推送全体有课成员的明日课程。

#### 取消群组提醒

```
取消绑定群提醒
```
取消本群的群组提醒功能。

### 四、小游戏

#### 猜数游戏

1. **开始游戏**
   ```
   @机器人 猜数
   ```
   启动 1-100 猜数字游戏。

2. **猜测数字**
   ```
   50
   ```
   直接发送数字进行猜测，机器人会提示"太大"、"太小"或"恭喜猜中"。

3. **退出游戏**
   ```
   @机器人 退出
   ```
   结束当前群的猜数游戏。

#### +1 击杀

当群内连续发送相同消息达到一定次数时，机器人会自动发送击杀图片。

### 五、基础交互

| 命令 | 说明 |
|------|------|
| `@机器人 hello` | 返回英文问候语 |
| `@机器人 你好` | 返回中文问候语 |
| `@机器人 1` | 返回 `true`（测试命令） |

## 👨‍💻 开发指南

### 架构设计

本项目采用**规则引擎**架构，核心组件包括：

1. **WebSocket 客户端**：负责与 NapCat 建立 WebSocket 连接，接收和发送消息。
2. **消息处理器**：解析接收到的群消息，提取关键信息。
3. **回复生成器**：基于规则引擎，匹配消息内容并生成回复。
4. **功能模块**：各个独立的功能模块（课表、游戏等），提供规则集。
5. **OneBot API 封装**：封装 OneBot v11 API 调用，简化发送消息操作。

### 添加新的回复规则

在 `src/core/reply_generator.cpp` 的 `default_rules` 中添加规则：

```cpp
{
    // 匹配函数：判断消息是否匹配该规则
    [](const json& msg_data, const std::string& content) {
        return is_at_bot(msg_data) && content == u8"你的关键词";
    },
    // 回复函数：生成回复内容
    [](const std::string& group_id, const std::string& content) {
        return u8"你的回复内容";
    }
}
```

### 添加新的功能模块

1. **创建模块文件**
   ```
   src/small_function/your_feature.cpp
   src/small_function/your_feature.h
   ```

2. **实现规则集**
   ```cpp
   // your_feature.h
   #pragma once
   #include <vector>
   #include "reply_generator.h"
   
   std::vector<ReplyRule> get_your_feature_rules();
   ```

   ```cpp
   // your_feature.cpp
   #include "your_feature.h"
   #include "utils.h"
   
   std::vector<ReplyRule> get_your_feature_rules() {
       return {
           {
               [](const json& msg_data, const std::string& content) {
                   return is_at_bot(msg_data) && content == u8"触发关键词";
               },
               [](const std::string& group_id, const std::string& content) {
                   // 你的逻辑
                   return u8"回复内容";
               }
           }
       };
   }
   ```

3. **在消息处理器中注册**
   在 `src/core/msg_handler.cpp` 中调用你的规则集：
   ```cpp
   #include "your_feature.h"
   
   // 在 handle_group_message 函数中
   auto your_rules = get_your_feature_rules();
   if (ReplyGenerator::generate_with_rules(msg_data, content, group_id, your_rules, reply)) {
       send_message(ws, reply);
       return;
   }
   ```

### 工具函数

#### 日志记录

```cpp
#include "utils.h"

write_log("这是一条日志");
write_log("错误信息: " + error_msg);
```

#### 编码转换

```cpp
#include "utils.h"

// GBK 转 UTF-8
std::string utf8_str = gbk_to_utf8("GBK编码的字符串");

// UTF-8 转 GBK
std::string gbk_str = utf8_to_gbk("UTF-8编码的字符串");
```

#### 检查是否 @机器人

```cpp
#include "utils.h"

if (is_at_bot(msg_data)) {
    // 消息中包含对机器人的 @
}
```

#### 构造 @消息

```cpp
#include "utils.h"

std::string qq_number = "123456789";
std::string message = with_at(qq_number, "你好！");
// 结果：[CQ:at,qq=123456789] 你好！
```

### 调试技巧

1. **查看日志**：所有操作都会记录到 `robot_log.txt`，出现问题时优先查看日志。
2. **测试消息**：使用测试命令 `@机器人 1` 验证机器人是否正常响应。
3. **WebSocket 连接**：确保 NapCat 正常运行，检查端口是否被占用。
4. **JSON 解析**：使用 nlohmann/json 的异常处理捕获解析错误。

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request！

### 提交 Issue

- 请详细描述问题或建议
- 如果是 Bug，请提供复现步骤和错误日志
- 如果是新功能建议，请说明使用场景

### 提交 Pull Request

1. Fork 本仓库
2. 创建你的特性分支：`git checkout -b feature/AmazingFeature`
3. 提交你的更改：`git commit -m 'Add some AmazingFeature'`
4. 推送到分支：`git push origin feature/AmazingFeature`
5. 提交 Pull Request

### 代码规范

- 遵循现有代码风格
- 添加必要的注释（中文）
- 确保代码能够编译通过
- 测试新功能是否正常工作

## 📄 许可证

本项目采用 [MIT License](LICENSE) 开源许可证。

## 🙏 致谢

- [NapCat](https://github.com/NapNeko/NapCatQQ) - 提供稳定的 QQ 协议实现和 OneBot 接口
- [Boost](https://www.boost.org/) - 强大的 C++ 网络和异步 I/O 库
- [nlohmann/json](https://github.com/nlohmann/json) - 简洁易用的 JSON 库
- [OneBot](https://github.com/botuniverse/onebot) - 统一的聊天机器人应用接口标准

## 📮 联系方式

- **项目地址**：https://github.com/Ningsui0714/qq-bot
- **Issues**：https://github.com/Ningsui0714/qq-bot/issues

## 📋 更新日志

### v1.0.0 (Current)

- ✅ 完整的课表管理系统
- ✅ 上课查询功能
- ✅ 智能提醒系统
- ✅ 猜数游戏和 +1 击杀
- ✅ 数据持久化
- ✅ 完善的日志系统

---

**注意**：本项目仅供学习和研究使用，请遵守相关法律法规和腾讯 QQ 的服务条款。
