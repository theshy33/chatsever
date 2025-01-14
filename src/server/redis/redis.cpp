#include "redis.h"
#include <muduo/base/Logging.h>
// #include <iostream>
// using namespace std;
Redis::Redis()
    :publishContext_(nullptr),subscribeContext_(nullptr)
{
}

Redis::~Redis()
{
    if(publishContext_ != nullptr)
    {
        redisFree(publishContext_);
    }
    if (subscribeContext_ != nullptr)
    {
        redisFree(subscribeContext_);
    }  
}

// 连接redis服务器
bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    publishContext_ = redisConnect("127.0.0.1", 6379);
    if (publishContext_ == nullptr || publishContext_->err)
    {
        // 释放连接资源
        if(publishContext_ != nullptr)
        {
            redisFree(publishContext_);
        }
        LOG_ERROR<<"connect to redis publish server failed."<<'\n';
        //cerr<<"Error: connect to redis publish server failed."<<endl;
        return false;
    }
    // 负责subscribe订阅消息的上下文连接
    subscribeContext_ = redisConnect("127.0.0.1", 6379);
    if (subscribeContext_ == nullptr || subscribeContext_->err)
    {
        // 释放连接资源
        if(subscribeContext_ != nullptr)
        {
            redisFree(subscribeContext_);
        }
        LOG_ERROR<<"connect to redis subscribe server failed."<<'\n';
        //cerr<<"Error: connect to redis subscribe server failed."<<endl;
        return false;
    }
    
    // 在单独的线程监听通道的事件，有消息到来时，调用业务层的回调函数
    thread t([this](){
        observerChannelMessage();
    });
    t.detach();
    LOG_INFO<<"Redis connection success."<<'\n';
    //cout<<"Redis connection success."<<endl;
    return true;
}
// 向redis指定通道channel发布消息
bool Redis::publish(string channel, string& message)
{
    redisReply* reply = (redisReply*)redisCommand(publishContext_, 
        "PUBLISH %s %s", channel.c_str(), message.c_str());
    if (nullptr == reply)
    {
        LOG_ERROR<<"publish message to redis failed."<<'\n';
        // cerr<<"Error: publish message to redis failed."<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
    
}
// 订阅redis指定通道channel
bool Redis::subscribe(string channel)
{
    // subscribe命令本身会阻塞，所以需要在单独的线程中执行，
    // 这里只订阅，不接受通道消息
    if (REDIS_ERR == redisAppendCommand(subscribeContext_, "SUBSCRIBE %s", channel.c_str()))
    {
        LOG_ERROR<<"subscribe command failed."<<'\n';
        // cerr<<"Error: subscribe command failed."<<endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区为空（done被设置为1)
    int done{0};
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(subscribeContext_, &done))
        {
            LOG_ERROR<<"subscribe write buffer failed."<<'\n';
            // cerr<<"Error: subscribe write buffer failed."<<endl;
            return false;
        }
    }
    LOG_INFO<<"Subscribe to redis channel "<<channel<<" success."<<'\n';
    // cout<<"Subscribe to redis channel "<<channel<<" success."<<endl;
    return true;
    
}
// 取消订阅redis指定通道channel
bool Redis::unsubscribe(string channel)
{
    if (REDIS_ERR == redisAppendCommand(subscribeContext_, "UNSUBSCRIBE %s", channel.c_str()))
    {
        LOG_INFO<<"unsubscribe command failed."<<'\n';
        // cerr<<"Error: unsubscribe command failed."<<endl;
        return false;
    }
    int done{0};
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(subscribeContext_, &done))
        {
            LOG_INFO<<"unsubscribe write buffer failed."<<'\n';
            // cerr<<"Error: unsubscribe write buffer failed."<<endl;
            return false;
        }
    }
    LOG_INFO<<"Unsubscribe to redis channel "<<channel<<" success."<<'\n';
    // cout<<"Unsubscribe to redis channel "<<channel<<" success."<<endl;
    return true;
    
}
// 在独立线程接收订阅消息
void Redis::observerChannelMessage()
{

    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->subscribeContext_, (void **)&reply))
    {
        // 订阅收到的消息是一个带三元素的数组
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息
            notifyMessageCallback_(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        else
        freeReplyObject(reply);
    }
    LOG_ERROR<<    ">>>>>>>>>>>>> observer_channel_message quit <<<"<<'\n';
    // cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;

}
// 初始化向业务层上报通道的回调函数
void Redis::setNotifyCallback(function<void(int, string)> callback)
{
    this->notifyMessageCallback_ = callback;
}
