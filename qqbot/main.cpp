// main.cpp：机器人主入口，负责连接NapCat并循环处理消息
#include <iostream>
#include "boost/asio.hpp"
#include "boost/beast.hpp"
#include "nlohmann/json.hpp"
#include "config.h"
#include "utils.h"
#include "group_message.h"

using namespace boost::asio;
using namespace boost::beast;
using json = nlohmann::json;
using tcp = boost::asio::ip::tcp;

int main() {
    try {
        // 写启动日志
        write_log("机器人启动，开始连接NapCat...");
        std::cout << "========================================" << std::endl;
        std::cout << "机器人正在启动... 机器人QQ：" << BOT_QQ << std::endl;
        std::cout << "连接NapCat WS：" << WS_HOST << ":" << WS_PORT << std::endl;
        std::cout << "========================================" << std::endl;

        // 1. 初始化WebSocket连接
        io_context ioc;
        tcp::resolver resolver(ioc);
        websocket::stream<tcp::socket> ws(ioc);

        // 2. 解析地址并连接
        auto results = resolver.resolve(WS_HOST, WS_PORT);
        net::connect(ws.next_layer(), results.begin(), results.end());

        // 3. WebSocket握手
        ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(http::field::user_agent, "C++ QQ Bot (v1)");
            }
        ));
        ws.handshake(WS_HOST, "/");

        // 连接成功日志
        std::string success_log = "WebSocket连接成功，开始监听群消息...";
        write_log(success_log);
        std::cout << success_log << std::endl;

        // 4. 主消息循环：接收消息并调用group_message处理函数
        flat_buffer buffer;
        while (true) {
            ws.read(buffer);
            std::string msg = buffers_to_string(buffer.data());
            buffer.consume(buffer.size()); // 清空缓冲区

            // 解析消息
            json msg_data = json::parse(msg);

            // 只处理群消息，调用group_message.h的处理函数
            if (msg_data["post_type"] == "message" && msg_data["message_type"] == "group") {
                handle_group_message(msg_data, ws);
            }
            else {
                write_log("收到非群消息，忽略：" + msg_data.dump(2));
            }
        }

        // 关闭连接（实际不会执行，因为是无限循环）
        ws.close(websocket::close_code::normal);
    }
    catch (const std::exception& e) {
        std::string err_log = "发生异常：" + std::string(e.what());
        write_log(err_log);
        std::cerr << "========================================" << std::endl;
        std::cerr << err_log << std::endl;
        std::cerr << "可能原因：NapCat未启动或端口错误" << std::endl;
        std::cerr << "========================================" << std::endl;
        return 1;
    }

    return 0;
}
