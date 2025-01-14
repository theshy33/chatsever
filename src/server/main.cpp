#include "chatserver.h"
#include "chatservice.h"
#include <iostream>
#include <signal.h>
using namespace std;

//处理服务器ctrl+c退出后，重置User状态
void resetHandler(int)
{
    ChatService::instance().reset();
    exit(0);
}
int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        cerr<<"command invalid!example:./server 192.168.70.133 6000"<<endl;
        exit(-1);
    }
    //解析命令行的参数传递的IP地址和端口号
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT, resetHandler); //设置信号处理函数

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();//开启事件循环
    return 0;
}