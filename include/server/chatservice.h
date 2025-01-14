#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include "public.hpp"
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "redis.h"
#include "usermodel.h"
#include "offlinemessagemodel.h"
#include "friendmodel.h"
#include "groupmodel.h"
#include "json.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

/// @brief 表示处理消息的事件回调方法类型
using MsgHandler = function<void(const TcpConnectionPtr &conn,json &js, Timestamp time)>;

/// @brief 聊天服务器业务类
class ChatService 
{
public:
    // 获取单例对象的接口函数
    static ChatService &instance();
    // 登录消息处理函数
    void login(const TcpConnectionPtr &conn,json &js, Timestamp time);
    // 登陆注销消息处理函数
    void loginout(const TcpConnectionPtr &conn,json &js, Timestamp time);
    // 注册消息处理函数
    void regist(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组
    void joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 发送群消息
    void sendGroupMsg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 退出群组
    void exitGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);


    // 处理客户端断开连接的异常
    void clientCloseException(const TcpConnectionPtr &conn);
    // 服务器异常，业务重置方法
    void reset();
    // 获取消息对应的处理器
    MsgHandler getHandler(MsgType msgId);

    void getRedisSubscribeMsg(int, string);

private:
    // 注册消息以及对应的Handler回调操作
    ChatService();
    // 存储消息id和对应的处理函数的映射
    unordered_map<MsgType, MsgHandler> msgHandlerMap_;
    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> userIdConnMap_;
    // 存在在线通信连接的用户id
    unordered_map<TcpConnectionPtr, int> userConnIdMap_;
    // 互斥锁,保证userIdConnMap_和userConnIdMap_的线程安全
    mutex mapMutex_;

    // 用户操作类对象
    UserModel userModel_;
    OfflineMsgModel offlineMsgModel_;
    FriendModel friendModel_;
    GroupModel groupModel_;
    // redis服务器操作类对象
    Redis redis_;
};

#endif // CHATSERVICE_H