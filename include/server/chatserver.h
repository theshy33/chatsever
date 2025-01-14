#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;


//服务器端类
class ChatServer
{
public:
    /// @brief 构造函数
    /// 初始化聊天服务器对象
    /// @param loop 指向事件循环对象的指针
    /// @param listenAddr 监听地址
    /// @param nameArg 服务器名称
    ChatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg);
    /// @brief 启动服务器
    void start();

private:
    /// @brief 处理新连接的回调函数
    /// 此函数用于处理新连接到服务器的请求，执行相应的逻辑。
    /// @param conn 指向连接对象的指针
    void onConnection(const TcpConnectionPtr &conn);
    
    /// @brief 处理新消息的回调函数
    /// 此函数用于处理从客户端接收到的新消息，执行相应的逻辑。
    /// @param conn 指向连接对象的指针
    /// @param buf 
    /// @param time
    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buffer,
                   Timestamp time);
    /// @brief 组合的muduo库，实现服务器端功能
    TcpServer server_;
    /// @brief 指向事件循环对象的指针
    EventLoop *loop_;
};
#endif
