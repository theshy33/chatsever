#ifndef PUBLIC_H
#define PUBLIC_H

/*
server和client共用的文件
*/
enum class MsgType
{
    LOGIN_MSG = 1,          //登录消息
    LOGIN_MSG_ACK,          //登录响应消息
    LOGINOUT_MSG,           // 注销消息
    REGISTER_MSG,           //注册消息
    REGISTER_MSG_ACK,       //注册响应消息
    ONE_CHAT_MSG,           //单聊消息
    ADD_FRIEND_MSG,         //添加好友消息
    ADD_FRIEND_MSG_ACK,     //添加好友响应消息

    CREATE_GROUP_MSG,       //创建群组消息
    CREATE_GROUP_MSG_ACK,   //创建群组响应消息
    JOIN_GROUP_MSG,         //加入群组消息
    JOIN_GROUP_MSG_ACK,     //加入群组响应消息
    GROUP_CHAT_MSG,         //群聊消息
    EXIT_GROUP_MSG,         //退出群组消息
};

#endif // PUBLIC_H
