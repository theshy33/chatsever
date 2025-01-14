#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
#include <string>
using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();
    // 连接redis服务器
    bool connect();
    // 向redis指定通道channel发布消息
    bool publish(string channel, string& message);
    // 订阅redis指定通道channel
    bool subscribe(string channel);
    // 取消订阅redis指定通道channel
    bool unsubscribe(string channel);
    // 在独立线程接收订阅消息
    void observerChannelMessage();
    // 初始化向业务层上报通道的回调函数
    void setNotifyCallback(function<void(int, string)> callback);
private:
    // hiredis同步上下文对象，负责publish消息
    redisContext *publishContext_;
    // hiredis同步上下文对象，负责订阅消息
    redisContext *subscribeContext_;
    // 回调操作，收到订阅的消息，给service层上报
    function<void(int, string)> notifyMessageCallback_;
};
#endif//REDIS_H
