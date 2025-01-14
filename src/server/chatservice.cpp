#include "chatservice.h"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
using namespace placeholders;
using namespace muduo;

/// @brief 获取单例对象的接口函数
/// @return 单例对象
ChatService &ChatService::instance()
{
    static ChatService service;
    return service;
}

/// @brief 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    //用户基本业务相关事件回调
    msgHandlerMap_.insert({MsgType::LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    msgHandlerMap_.insert({MsgType::LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    msgHandlerMap_.insert({MsgType::REGISTER_MSG, std::bind(&ChatService::regist, this, _1, _2, _3)});
    msgHandlerMap_.insert({MsgType::ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    msgHandlerMap_.insert({MsgType::ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    //群聊业务相关事件回调
    msgHandlerMap_.insert({MsgType::CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    msgHandlerMap_.insert({MsgType::JOIN_GROUP_MSG, std::bind(&ChatService::joinGroup, this, _1, _2, _3)});
    msgHandlerMap_.insert({MsgType::GROUP_CHAT_MSG, std::bind(&ChatService::sendGroupMsg, this, _1, _2, _3)});
    msgHandlerMap_.insert({MsgType::EXIT_GROUP_MSG, std::bind(&ChatService::exitGroup, this, _1, _2, _3)});

    // 连接redis服务器
    if(redis_.connect())
    {
        LOG_INFO << "redis connect success";
        // 设置上报消息接收回调
        redis_.setNotifyCallback(std::bind(&ChatService::getRedisSubscribeMsg, this, _1, _2));
    }
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 重置用户状态为offline
    userModel_.resetState();
    // 重置connId和userId的映射关系
    {
        lock_guard<mutex> lock(mapMutex_);
        userConnIdMap_.clear();
    }
    {
        lock_guard<mutex> lock(mapMutex_);
        userIdConnMap_.clear();
    }
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(MsgType msgId)
{
    auto it = msgHandlerMap_.find(msgId);
    // 记录错误日志,msgid没有对应的处理器
    if (it == msgHandlerMap_.end())
    {
        // LOG_ERROR << "msgid:" << (int)msgId << " can not find handler";
        // return nullptr;
        //  改为返回一个空的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid:" << (int)msgId << " can not find handler!";
        };
    }
    return it->second;
}

// 处理业务
/// @brief 登录消息处理函数
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // LOG_INFO << "login msg recv:" << js.dump();
    //  id password
    int id = js["id"];
    string password = js["password"];

    User user = userModel_.queryUser(id);
    if (user.getId() != -1 && user.getPassword() == password)
    {
        json response;
        response["msgid"] = static_cast<int>(MsgType::LOGIN_MSG_ACK);
        if (user.getState() == "online")
        {
            // 用户已经在线,不允许重复登录
            response["error"] = 3; // 失败,重复登陆
            response["errmsg"] = "user already online, please login new one";
        }
        else//登陆成功
        {
            // 记录用户连接关系
            {
                lock_guard<mutex> lock(mapMutex_);
                userConnIdMap_.insert({conn, id});
            }
            {
                lock_guard<mutex> lock(mapMutex_);
                userIdConnMap_.insert({id, conn});
            }
            // 登陆成功，向redis订阅channel(id)
            redis_.subscribe(to_string(id));

            // 更新用户状态 state offline->online
            user.setState("online");
            userModel_.updateUserState(user);

            response["error"] = 0; // 成功为0
            response["id"] = user.getId();
            response["name"] = user.getName();

            //检查是否有离线消息
            vector<string> offlinemsg = offlineMsgModel_.query(id);
            if (!offlinemsg.empty())
            {
                //有离线消息,发送给客户端
                response["offlinemsg"] = offlinemsg;
                //清空离线消息
                offlineMsgModel_.remove(id);
            }
            // 查询该用户的好友列表并返回
            vector<User> friendVec = friendModel_.query(id);
            if(!friendVec.empty())
            {
                vector<string> vj;
                for(auto& friendUser : friendVec)
                {
                    json js;
                    js["id"] = friendUser.getId();
                    js["name"] = friendUser.getName();
                    js["state"] = friendUser.getState();
                    vj.push_back(js.dump());
                }
                response["friends"] = move(vj);
            }

            //查询用户的群组列表并返回
            vector<Group> groupVec = groupModel_.queryGroups(id);
            if (!groupVec.empty())
            {
                vector<string> groupVecStr;
                for(auto& group : groupVec)
                {
                    json groupjs;
                    groupjs["id"] = group.getId();
                    groupjs["groupname"] = group.getName();
                    groupjs["groupdesc"] = group.getDesc();
                    vector<string> userVecStr;
                    for (auto& groupuser : group.getUsers())
                    {
                        json userjs;
                        userjs["id"] = groupuser.getId();
                        userjs["name"] = groupuser.getName();
                        userjs["state"] = groupuser.getState();
                        userjs["role"] = groupuser.getRole();
                        userVecStr.emplace_back(userjs.dump());
                    }
                    groupjs["users"] = userVecStr;
                    groupVecStr.emplace_back(groupjs.dump());
                }
                response["groups"] = move(groupVecStr);
            }
        }
        conn->send(response.dump());
    }
    else
    {
        // 用户不存在或密码错误
        json response;
        response["msgid"] = static_cast<int>(MsgType::LOGIN_MSG_ACK);
        if (user.getId() == -1)
        {
            response["error"] = 1; // 失败,用户不存在
            response["errmsg"] = "user not exist";
        }
        else
        {
            response["error"] = 2; // 失败,密码错误
            response["errmsg"] = "password error";
        }
        conn->send(response.dump());
    }
}

// 登陆注销消息处理函数
void ChatService::loginout(const TcpConnectionPtr &conn,json &js, Timestamp time)
{
    int userId = js["id"].get<int>();
    TcpConnectionPtr userConn{nullptr};
    {
        lock_guard<mutex> lock(mapMutex_);
        auto it = userIdConnMap_.find(userId);
        if (it != userIdConnMap_.end())
        {
            userConn = it->second;
            userIdConnMap_.erase(it);
        }
    }
    if (userConn)
    {
        lock_guard<mutex> lock(mapMutex_);
        auto it = userConnIdMap_.find(userConn);
        if (it != userConnIdMap_.end())
        {
            userConnIdMap_.erase(it);
        }
    }

    // 注销用户,相当于下线，在redis里面取消订阅channel(id)
    redis_.unsubscribe(to_string(userId));

    // 更新用户状态 state online->offline
    User user(userId, "", "", "offline");
    userModel_.updateUserState(user);
}

/// @brief 注册消息处理函数
void ChatService::regist(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // LOG_INFO << "regist msg recv:" << js.dump();
    //  name,password
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);

    json response;
    response["msgid"] = static_cast<int>(MsgType::REGISTER_MSG_ACK);

    if (userModel_.insertUser(user))
    {
        // 注册成功
        response["error"] = 0; // 成功为0
        response["id"] = user.getId();
    }
    else
    {
        // 注册失败
        response["error"] = 1; // 成功为0
        response["errmsg"] = "regist failed";
    }
    conn->send(response.dump());
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["to"].get<int>();
    {
        lock_guard<mutex> lock(mapMutex_);
        auto it = userIdConnMap_.find(toid);
        if(it != userIdConnMap_.end())
        {
            //toid在线当前服务器,发送消息 服务器主动推送消息
            it->second->send(js.dump());
            return;
        }
    }
    // 查询是否在线，在其他服务器
    User user = userModel_.queryUser(toid);
    
    LOG_INFO<<"check:user: "<<user.getId()<<" "<<user.getState()<<'\n';
    if(user.getState() == "online")//在线其他服务器
    {
        // 向redis发布消息，channel(toid),message(js.dump())
        string messages = js.dump();
        redis_.publish(to_string(toid), messages);
        return;
    }
    //toid不在线,保存消息到数据库,等待对方上线后发送
    offlineMsgModel_.insert(toid,js.dump());
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    json response;
    response["msgid"] = static_cast<int>(MsgType::ADD_FRIEND_MSG_ACK);
    //存储好友信息
    if(friendModel_.insert(userid, friendid))
    {
        //存储好友信息成功
        response["error"] = 0; // 成功为0
    }
    else
    {
        //存储好友信息失败
        response["error"] = 1; // 失败
        response["errmsg"] = "add friend failed";
    }
    conn->send(response.dump());
}

// 创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];

    //存储群组信息
    Group group(-1, groupname, groupdesc);
    json response;
    response["msgid"] = static_cast<int>(MsgType::CREATE_GROUP_MSG_ACK);
    //创建群组
    if(groupModel_.createGroup(group))
    {
        //创建群组成功
        // 存储创建人信息
        groupModel_.joinGroup(userid, group.getId(),"creator");
        response["error"] = 0; // 成功为0
        response["groupid"] = group.getId();
    }
    else
    {
        //创建群组失败
        response["error"] = 1; // 失败
        response["errmsg"] = "create group failed";
    }
    conn->send(response.dump());
}
// 加入群组
void ChatService::joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    json response;
    response["msgid"] = static_cast<int>(MsgType::JOIN_GROUP_MSG_ACK);
    //加入群组
    if(groupModel_.joinGroup(userid, groupid, "normal"))
    {
        //加入群组成功
        response["error"] = 0; // 成功为0
    }
    else
    {
        //加入群组失败
        response["error"] = 1; // 失败
        response["errmsg"] = "join group failed";
    }
    conn->send(response.dump());
}
// 发送群消息
void ChatService::sendGroupMsg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = js["id"].get<int>();
    int groupId = js["groupid"].get<int>();
    string msg = js["msg"];
    //发送群消息
    vector<int> userIdVec = groupModel_.queryGroupUsers(userId,groupId);
    
    lock_guard<mutex> lock(mapMutex_);
    for (auto& id : userIdVec)
    {
        auto it = userIdConnMap_.find(id);
        if (it != userIdConnMap_.end())
        {
            //toid在线,发送消息 服务器主动推送消息
            it->second->send(js.dump());
        }
        else
        {
            // 查询是否在线，在其他服务器
            User user = userModel_.queryUser(id);
            if(user.getState() == "online")//在线其他服务器
            {
                // 向redis发布消息，channel(toid),message(js.dump())
                string messages = js.dump();
                redis_.publish(to_string(id), messages);
            }
            else//toid不在线,保存消息到数据库,等待对方上线后发送
            {
                offlineMsgModel_.insert(id, js.dump());
            } 
        }
    }
}
// 退出群组
void ChatService::exitGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    //退出群组
    groupModel_.exitGroup(userid, groupid);
}

// 处理客户端断开连接的异常
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    int id{-1};
    {
        lock_guard<mutex> lock(mapMutex_);
        auto it = userConnIdMap_.find(conn);
        if (it != userConnIdMap_.end())
        {
            // 从map表删除用户链接信息
            id = it->second;
            userConnIdMap_.erase(it);
        }
    }
    {
        lock_guard<mutex> lock(mapMutex_);
        auto it = userIdConnMap_.find(id);
        if (it != userIdConnMap_.end())
        {
            // 从map表删除用户链接信息
            userIdConnMap_.erase(it);
        }
    }
    if (id != -1)
    {
        // 注销用户,相当于下线，在redis里面取消订阅channel(id)
        redis_.unsubscribe(to_string(id));

        // 更新用户状态 state online->offline
        user.setId(id);
        user.setState("offline");
        userModel_.updateUserState(user);
    }
}

// 从redis消息队列中获取订阅的消息
void ChatService::getRedisSubscribeMsg(int userId,string msg)
{
    lock_guard<mutex> lock(mapMutex_);
    auto it = userIdConnMap_.find(userId);
    if (it != userIdConnMap_.end())
    {
        // 用户在线,发送消息
        it->second->send(msg);
        return;
    }
    // 用户不在线,保存消息到数据库,等待对方上线后发送
    offlineMsgModel_.insert(userId,msg);
}
