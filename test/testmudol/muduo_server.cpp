/*
muduo网络库提供了两个主要类
TcpServer : 用于编写服务端
TcpClient : 用于编写客户端

epoll + 线程池
//好处：能够把I/O的代码和业务代码区分开
        用户的连接和断开    用户的可读写事件
*/
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders; // bind,_1

/// @brief 基于muduo网络库开发服务器程序
/// 1.组合TcpServer对象
/// 2.创建EventLoop事件循环对象的指针
/// 3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
/// 4.在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写事件的回调函数
/// 5.设置合适的服务端线程数量，muduo会自己分配I/O线程和worker线程
class ChatServer
{
public:
    /**
     * @brief 构造函数
     * @param loop 事件循环对象指针
     * @param listenAddr 监听的IP+Port
     * @param nameArg 服务器的名字
     */
    ChatServer(EventLoop *loop,               
               const InetAddress &listenAddr, 
               const string &nameArg)         
        : server_(loop, listenAddr, nameArg), loop_(loop)
    {
        // 给服务器注册用户连接的创建和断开回调
        server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

        // 给服务器注册用户读写事件回调
        server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        //设置设置服务端的线程数量,一个I/O线程，2个worker线程
        server_.setThreadNum(3);
    }
    //开启事件循环
    void Start()
    {
        server_.start();
    }

private:
    // 专门处理用户的连接创建和断开  epoll listenfd  accept
    void onConnection(const TcpConnectionPtr &conn)
    {
        if(conn->connected())//连上了
        {
            cout << conn->peerAddress().toIpPort() << "->" << 
                conn->localAddress().toIpPort() << " stateon"<<endl;
        }else
        {
            cout << conn->peerAddress().toIpPort() << "->" << 
            conn->localAddress().toIpPort() << " stateoff" << endl;
            conn->shutdown();   //close(fd)
            //loop_->quit();
        }
    }
    //专门处理用户读写事件
    void onMessage(const TcpConnectionPtr &conn,    //连接
                   Buffer *buffer,    //缓冲区
                   Timestamp time)   //时间信息
    {
        string buf = buffer->retrieveAllAsString();//全部取出
        cout << "recv data:" << buf << "time:" << time.toString() << endl;
        conn->send(buf);//发送
    }
    TcpServer server_; // #1
    EventLoop *loop_;  // #2 epoll
};

int main()
{
    EventLoop Loop; // epoll
    InetAddress addr("192.168.59.129", 6000);
    ChatServer server(&Loop, addr, "ChatServer");
    server.Start(); // listenfd epoll_ctl=>epoll
    Loop.loop();    // epoll_wait以阻塞方式等待新用户连接，已连接用户读写事件等
    return 0;
}
