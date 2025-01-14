#include "chatserver.h"
#include "json.hpp"
#include "chatservice.h"
#include <functional>
#include <string>
#include <thread>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;// 这里的json是nlohmann::json

/// @brief 构造函数
/// 初始化聊天服务器对象
/// @param loop 指向事件循环对象的指针
/// @param listenAddr 监听地址
/// @param nameArg 服务器名称
ChatServer::ChatServer(EventLoop *loop, 
                        const InetAddress &listenAddr, 
                        const string &nameArg)
    : server_(loop, listenAddr, nameArg)
{
    // 注册链接回调函数
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调函数
    server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    //获取cpu核数
    //int cpuNum = std::thread::hardware_concurrency();
    // 设置线程数
    server_.setThreadNum(4); 
}

/// @brief 启动服务器
void ChatServer:: start()
{
    server_.start();
}

/// @brief 处理新连接的回调函数
/// 此函数用于处理新连接到服务器的请求，执行相应的逻辑。
/// @param conn 指向连接对象的指针
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if(!conn->connected())
    {
        // 连接断开
        ChatService::instance().clientCloseException(conn);
        conn->shutdown();
    }
}

/// @brief 处理新消息的回调函数
/// 此函数用于处理从客户端接收到的新消息，执行相应的逻辑。
/// @param conn 指向连接对象的指针
/// @param buf
/// @param time
void ChatServer::onMessage(const TcpConnectionPtr &conn,
               Buffer *buffer,
               Timestamp time)
{
    string buf=buffer->retrieveAllAsString();
    // 数据反序列化
    json js=json::parse(buf);
    // 达到的目的：完全解耦网络模块和业务模块的代码
    // 通过js["msgid"] 获取-->业务hander=> conn js time
    auto msgHandler = ChatService::instance().getHandler(static_cast<MsgType>(js["msgid"].get<int>()));
    // 回调消息绑定好的处理器，来执行相应的业务逻辑
    msgHandler(conn, js, time);
}
