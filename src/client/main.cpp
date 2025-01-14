#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
// time
#include <ctime>
#include <sstream>
#include <iomanip>

#include <unordered_map>
#include <functional>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 记录当前系统登陆的用户信息
User currentUser;
// 记录当前用户的好友列表信息
vector<User> friendList;
// 记录当前用户的群组列表信息
vector<Group> groupList;

// 接收线程
void recvThread(int clientfd);
// 获取系统当前时间
string getCurrentTime();
// 主聊天页面
void mainMenu(int);
// 记录登陆的基本信息，好友列表，群组列表，离线消息
void saveUserInfo(json& recvJson);
// 显示当前用户的基本信息
void showUserInfo(json& recvJson);

// 控制聊天页面程序
bool isMainMenuRunning{false};

// 主函数
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid!,example:./client ip port" << endl;
        exit(-1); // 退出程序
    }
    // 解析通过命令行传入的ip和port
    string ip = argv[1];
    int port = atoi(argv[2]);
    // 创建socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        cerr << "create socket failed!" << endl;
        exit(-1);
    }
    // 绑定ip和port
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 连接服务器
    if (connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        cerr << "connect server failed!" << endl;
        exit(-1);
    }
    // main函数用于接收用户输入并处理
    while (true)
    {
        // 显示首页菜单，登陆/注册/退出
        cout << "=============================" << endl;
        cout << "1.login" << endl;
        cout << "2.register" << endl;
        cout << "3.exit" << endl;
        cout << "=============================" << endl;
        cout << "choice: ";
        int choice{0};
        try
        {
            cin >> choice;
            if (cin.fail())
            {
                throw invalid_argument("输入无效，请输入数字！");
            }
        }
        catch (const invalid_argument &e)
        {
            cout << e.what() << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }

        cin.get(); // 去掉回车符
        switch (choice)
        {
        case 1: // login业务
        {
            // 输入id和密码
            int id{0};
            string password;
            cout << "input id: ";
            cin >> id;
            cout << "input password: ";
            cin.get(); // 去掉回车符
            getline(cin, password);

            // 发送登陆请求
            json req;
            req["msgid"] = static_cast<int>(MsgType::LOGIN_MSG);
            req["id"] = id;
            req["password"] = password;
            string reqStr = req.dump();
            int len = send(clientfd, reqStr.c_str(), reqStr.length() + 1, 0);
            if (len == -1)
            {
                cerr << "send failed!" << reqStr << endl;
            }
            else
            {
                // 接收服务器返回的消息
                char recvBuf[4096] = {0};
                len = recv(clientfd, recvBuf, sizeof(recvBuf), 0);
                if (len == -1)
                {
                    cerr << "recv login failed!" << endl;
                }
                else
                {
                    //cout<<"check: login recv : "<<recvBuf<<endl;
                    json recvJson = json::parse(recvBuf);
                    if (recvJson["error"] == 1)
                    {
                        cerr << "login failed!,the id is not exist" << endl;
                    }
                    else if (recvJson["error"] == 2)
                    {
                        cerr << "login failed!,the password is wrong" << endl;
                    }
                    else if (recvJson["error"] == 0)
                    {  
                        // 登陆成功，保存用户信息，好友列表，群组列表
                        saveUserInfo(recvJson);
                        // 显示当前用户的基本信息
                        showUserInfo(recvJson);
                        // 登陆成功，启动接收线程负责接收服务器的消息
                        int static threadCount {0};
                        if(threadCount == 0)
                        {
                            std::thread recvTask(recvThread, clientfd);
                            recvTask.detach();
                            threadCount++;
                        }
                        // 进入主聊天页面
                        isMainMenuRunning = true;
                        mainMenu(clientfd);
                    }
                    else
                    {
                        cerr << "login failed!,unknown error" << endl;
                    }
                }
            }
        }
        break;
        case 2: // register业务
        {
            string name;
            string password;
            cout << "input name: ";
            getline(cin, name);
            cout << "input password: ";
            getline(cin, password);

            // cout<<"check: register : "<<name<<" "<<password<<endl;
            //  发送注册请求
            json req;
            req["msgid"] = (int)MsgType::REGISTER_MSG;
            req["name"] = name;
            req["password"] = password;
            string reqStr = req.dump();

            // cout<<"check: register req : "<<reqStr<<endl;
            // return 0;
            //
            int len = send(clientfd, reqStr.c_str(), reqStr.length() + 1, 0);
            if (len == -1)
            {
                cerr << "send failed!" << reqStr << endl;
            }
            else
            {
                char recvBuf[1024] = {0};
                len = recv(clientfd, recvBuf, sizeof(recvBuf), 0);
                if (len == -1)
                {
                    cerr << "recv failed!" << endl;
                }
                else
                {
                    json recvJson = json::parse(recvBuf);
                    // cout<<"check: register recv : "<<recvJson.dump()<<endl;
                    if (recvJson["error"].get<int>() != 0) // 注册失败
                    {
                        cerr << name << " is already exist!" << endl;
                    }
                    else // 注册成功
                    {
                        cout << name << "register success!, userid is " << recvJson["id"]
                             << ", please do not forget it!" << endl;
                    }
                }
            }
        }
        break;
        case 3: // exit业务
        {
            close(clientfd);
            exit(0);
        }
        break;
        default: // 其他情况
        {
            cout << "invalid choice!" << endl;

            break;
        }
        }
    }

    cout << getCurrentTime() << " client start..." << endl;
    return 0;
}

// 保存用户信息，好友列表，群组列表
void showUserInfo(json& recvJson)
{
    cout << "==========login user info==========" << endl;
    cout << "userid: " << currentUser.getId() << '\t';
    cout << "username: " << currentUser.getName() << endl;
    // 显示好友列表
    cout << "==========friend list==========" << endl;
    for (auto &user : friendList)
    {
        cout << user.getId() << '\t' << user.getName() << "\t" << user.getState() << endl;
    }
    // 显示群组列表
    cout << "==========group list==========" << endl;
    for (auto &group : groupList)
    {
        cout << "group: ";
        cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
        for (auto &user : group.getUsers())
        {
            cout << "user: " << user.getId() << " " << user.getName() << " "
                 << user.getState() << " " << user.getRole() << endl;
        }
    }
    // 显示当前用户的离线消息，个人聊天消息或者群组聊天消息
    if (recvJson.contains("offlinemsg"))
    {
        cout << "==========offlinemsg==========" << endl;
        vector<string> offlienMsgs = recvJson["offlinemsg"];
        for (auto &msgStr : offlienMsgs)
        {
            json msgJson = json::parse(msgStr);
            // time + [id] +name + "said: " + msg
            if (msgJson.contains("groupid")) // 群聊天消息
            {
                cout << msgJson["time"] << endl
                     << " [" << "groupid:" << msgJson["groupid"] << "]"
                     << " [" << "userid: " << msgJson["id"] << "]"
                     << " " << msgJson["name"] << " said: " << msgJson["msg"] << endl;
            }
            else // 个人聊天消息
            {
                cout << msgJson["time"] << endl
                     << " [userid: " << msgJson["id"] << "] "
                     << msgJson["name"] << " said: " << msgJson["msg"] << endl;
            }
        }
    }
    cout << "==========end==========" << endl;
}

// 显示登陆的基本信息，好友列表，群组列表，离线消息
void saveUserInfo(json &recvJson)
{
    // 初始化清空用户信息
    friendList.clear();
    groupList.clear();

    // 登陆成功，更新当前用户信息
    currentUser.setId(recvJson["id"].get<int>());
    currentUser.setName(recvJson["name"]);

    // 记录好友列表
    if (recvJson.contains("friends"))
    {
        vector<string> friends = recvJson["friends"];
        for (auto &frinedStr : friends)
        {
            json userJson = json::parse(frinedStr);
            User user;
            user.setId(userJson["id"].get<int>());
            user.setName(userJson["name"]);
            user.setState(userJson["state"]);
            friendList.emplace_back(user);
        }
    }

    // 记录群组列表
    if (recvJson.contains("groups"))
    {
        vector<string> groups = recvJson["groups"];
        for (auto &groupStr : groups)
        {
            json groupJson = json::parse(groupStr);
            Group group;
            group.setId(groupJson["id"].get<int>());
            group.setName(groupJson["groupname"]);
            group.setDesc(groupJson["groupdesc"]);

            vector<string> users = groupJson["users"];
            for (auto &userStr : users)
            {
                json userJson = json::parse(userStr);
                GroupUser user;
                user.setId(userJson["id"].get<int>());
                user.setName(userJson["name"]);
                user.setState(userJson["state"]);
                user.setRole(userJson["role"]);
                group.getUsers().emplace_back(user);
            }
            groupList.emplace_back(group);
        }
    }
}
// 子线程-接收线程
void recvThread(int clientfd)
{
    // 循环接收消息
    while (true)
    {
        char recvBuf[1024] = {0};
        int len = recv(clientfd, recvBuf, sizeof(recvBuf), 0);
        if (len == -1 || len == 0)
        {
            cout << "recvThread recv failed!" << endl;
            close(clientfd);
            exit(-1);
        }
        if (len == sizeof(recvBuf))
        {
            cerr << "recvThread recv buf too long!" << endl;
            continue;
        }
        recvBuf[len] = '\0';
        json recvJson;
        try
        {
            // 解析json消息
            recvJson = json::parse(recvBuf);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            cerr << "recvJson parse error" << endl;
            continue;
        }

        try
        {
            // 解析json消息
            //json recvJson = json::parse(recvBuf);
            // 处理消息
            // cout<<"check: recv : "<<recvJson.dump()<<endl;
            MsgType msgId = static_cast<MsgType>(recvJson["msgid"].get<int>());
            switch (msgId)
            {
                case MsgType::LOGIN_MSG_ACK: // 登陆响应
                {
                    if (recvJson["error"].get<int>() != 0)
                    {
                        cerr << recvJson["errmsg"] << endl;
                    }
                    else
                    {
                        cout << "login success!" << endl;
                    }
                    break;
                }
                case MsgType::REGISTER_MSG_ACK: // 注册响应
                {
                    if (recvJson["error"].get<int>() != 0)
                    {
                        cerr << recvJson["errmsg"] << endl;
                    }
                    else
                    {
                        cout << "register success!,"
                            <<"userid is "<<recvJson["id"]
                            <<" please do not forget it!" << endl;
                    }
                    break;
                }
                case MsgType::ONE_CHAT_MSG: // 个人聊天消息
                {
                    cout << recvJson["time"] << endl
                        << " [userid:" << recvJson["id"] << "] "
                        << recvJson["name"] << " said: " << recvJson["msg"]
                        << endl;
                    break;
                }
                case MsgType::GROUP_CHAT_MSG: // 群聊天消息
                {
                    cout << recvJson["time"] << endl
                        << " [" << "groupid:" << recvJson["groupid"] << "]"
                        << " [" << "userid: " << recvJson["id"] << "] "
                        << recvJson["name"] << " said: " << recvJson["msg"]
                        << endl;
                    break;
                }
                case MsgType::ADD_FRIEND_MSG_ACK: // 添加好友响应
                {
                    if (recvJson["error"].get<int>() != 0)
                    {
                        cerr << recvJson["errmsg"] << endl;
                    }
                    else
                    {
                        cout << "add friend success!" << endl;
                    }
                    break;
                }  
                case MsgType::CREATE_GROUP_MSG_ACK: // 创建群组响应
                {
                    if(recvJson["error"].get<int>() != 0)
                    {
                        cerr << recvJson["errmsg"] << endl;
                    }
                    else
                    {
                        cout << "create group success!,this is groupid: " << recvJson["groupid"]
                             << ", please do not forget it!" << endl;
                    }
                    break;
                }
                case MsgType::JOIN_GROUP_MSG_ACK: // 加入群组响应
                {
                    if(recvJson["error"].get<int>() != 0)
                    {
                        cerr << recvJson["errmsg"] << endl;
                    }
                    else
                    {
                        cout << "join group success!" << endl;
                    }
                    break;
                }
                default: // 其他情况
                {
                    cerr << "unknown msgid: " << static_cast<int>(msgId) << endl;
                    break;
                }
            }
        }
        catch (const json::parse_error &e)
        {
            cerr << "JSON parse error: " << e.what() << endl;
            cerr << "Received JSON: " << recvJson.dump() << endl;
        }
        catch (const json::type_error &e)
        {
            cerr << "JSON type error: " << e.what() << endl;
            cerr << "Received JSON: " << recvJson.dump() << endl;
        }
        catch (const std::exception &e)
        {
            cerr << "Unexpected error: " << e.what() << endl;
        }
    }
}

// "help"command handler
void help(int clientfd = 0, string args = "");
// "chat"command handler
void chat(int, string);
// "groupchat"command handler
void groupchat(int, string);
// "addfriend"command handler
void addfriend(int, string);
// "creategroup"command handler
void creategroup(int, string);
// "joingroup"command handler
void joingroup(int, string);
// "exitgroup"command handler
void exitgroup(int, string);
// "loginout"command handler
void loginout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> clientCmdList = {
    {"help", "显示客户端命令列表"},
    {"chat", "一对一聊天,格式 chat:frinedid:message"},
    {"groupchat", "群聊天,格式 groupchat:groupid:message"},
    {"addfriend", "添加好友,格式 addfriend:friendid"},
    {"creategroup", "创建群组,格式 creategroup:groupname:groupdesc"},
    {"joingroup", "加入群组,格式 joingroup:groupid"},
    {"exitgroup", "退出群组,格式 exitgroup:groupid"},
    {"loginout", "退出系统,格式 logout"}};

// 注册系统支持的客户端命令处理函数
unordered_map<string, function<void(int, string)>> clientCmdHandler = {
    {"help", help},
    {"chat", chat},
    {"groupchat", groupchat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"joingroup", joingroup},
    {"exitgroup", exitgroup},
    {"loginout", loginout}};

// 处理客户端输入命令
void help(int, string)
{
    cout << "show command list:>>> " << endl;
    for (auto &cmd : clientCmdList)
    {
        cout << cmd.first << ":" << cmd.second << endl;
    }
    cout << endl;
}
// 一对一聊天
void chat(int clientfd, string args)
{
    // friendid:message
    int idx = args.find(":");
    if (idx == string::npos)
    {
        cerr << "chat command format error!" << endl;
        return;
    }
    int friendid = stoi(args.substr(0, idx));
    string message = args.substr(idx + 1);
    if (message.empty())
    {
        cerr << "chat message is empty!" << endl;
        return;
    }
    // 发送聊天消息
    json req;
    req["msgid"] = static_cast<int>(MsgType::ONE_CHAT_MSG);
    req["id"] = currentUser.getId();
    req["name"] = currentUser.getName();
    req["to"] = friendid;
    req["msg"] = message;
    req["time"] = getCurrentTime();
    string reqStr = req.dump();
    int len = send(clientfd, reqStr.c_str(), reqStr.size() + 1, 0);
    if (-1 == len)
    {
        cerr << "send chat msg failed!-> " << reqStr << endl;
    }
}
// 群聊天
void groupchat(int clientfd, string args)
{
    // groupid:message
    int idx = args.find(":");
    if (idx == string::npos)
    {
        cerr << "groupchat command format error!" << endl;
        return;
    }
    int groupid = stoi(args.substr(0, idx));
    string message = args.substr(idx + 1);
    if (message.empty())
    {
        cerr << "groupchat message is empty!" << endl;
        return;
    }
    // 发送群聊消息
    json req;
    req["msgid"] = static_cast<int>(MsgType::GROUP_CHAT_MSG);
    req["id"] = currentUser.getId();
    req["name"] = currentUser.getName();
    req["groupid"] = groupid;
    req["msg"] = message;
    req["time"] = getCurrentTime();
    string reqStr = req.dump();
    int len = send(clientfd, reqStr.c_str(), reqStr.size() + 1, 0);
    if (-1 == len)
    {
        cerr << "send groupchat msg failed!-> " << reqStr << endl;
    }
}
// 添加好友
void addfriend(int clientfd, string args)
{
    // friendid
    int friendid = stoi(args);
    // 发送添加好友请求
    json req;
    req["msgid"] = static_cast<int>(MsgType::ADD_FRIEND_MSG);
    req["id"] = currentUser.getId();
    req["name"] = currentUser.getName();
    req["friendid"] = friendid;
    string reqStr = req.dump();
    int len = send(clientfd, reqStr.c_str(), reqStr.size() + 1, 0);
    if (-1 == len)
    {
        cerr << "send addfriend msg failed!-> " << reqStr << endl;
    }
}
// 创建群组
void creategroup(int clientfd, string args)
{
    // groupname:groupdesc
    int idx = args.find(":");
    if (idx == string::npos)
    {
        cerr << "creategroup command format error!" << endl;
        return;
    }
    string groupname = args.substr(0, idx);
    string groupdesc = args.substr(idx + 1);
    // 发送创建群组请求
    json req;
    req["msgid"] = static_cast<int>(MsgType::CREATE_GROUP_MSG);
    req["id"] = currentUser.getId();
    req["groupname"] = groupname;
    req["groupdesc"] = groupdesc;
    string reqStr = req.dump();
    int len = send(clientfd, reqStr.c_str(), reqStr.size() + 1, 0);
    if (-1 == len)
    {
        cerr << "send creategroup msg failed!-> " << reqStr << endl;
    }
}
// 加入群组
void joingroup(int clientfd, string args)
{
    // groupid
    int groupid = stoi(args);
    // 发送加入群组请求
    json req;
    req["msgid"] = static_cast<int>(MsgType::JOIN_GROUP_MSG);
    req["id"] = currentUser.getId();
    req["groupid"] = groupid;
    string reqStr = req.dump();
    int len = send(clientfd, reqStr.c_str(), reqStr.size() + 1, 0);
    if (-1 == len)
    {
        cerr << "send joingroup msg failed!-> " << reqStr << endl;
    }
}
// 退出群组
void exitgroup(int clientfd, string args)
{
    // groupid
    int groupid = stoi(args);
    // 发送退出群组请求
    json req;
    req["msgid"] = static_cast<int>(MsgType::EXIT_GROUP_MSG);
    req["id"] = currentUser.getId();
    req["groupid"] = groupid;
    string reqStr = req.dump();
    int len = send(clientfd, reqStr.c_str(), reqStr.size() + 1, 0);
    if (-1 == len)
    {
        cerr << "send exitgroup msg failed!-> " << reqStr << endl;
    }
}
// 退出登陆
void loginout(int clientfd, string)
{
    // 发送退出登陆请求
    json req;
    req["msgid"] = static_cast<int>(MsgType::LOGINOUT_MSG);
    req["id"] = currentUser.getId();
    string reqStr = req.dump();

    int len = send(clientfd, reqStr.c_str(), reqStr.size() + 1, 0);
    if (-1 == len)
    {
        cerr << "send loginout msg failed!-> " << reqStr << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}
    

// 主聊天页面
void mainMenu(int clientfd)
{
    help();
    string cmdMsg;
    while (isMainMenuRunning)
    {
        getline(cin, cmdMsg);
        // 处理输入命令
        string command; // 存储命令
        int idx = cmdMsg.find(":");
        if (idx == string::npos) // 可能是help 和 loginout
        {
            command = cmdMsg;
        }
        else
        {
            command = cmdMsg.substr(0, idx);
        }
        auto it = clientCmdHandler.find(command);
        if (it == clientCmdHandler.end())
        {
            cerr << "invalid command!" << endl;
            continue;
        }
        // 调用对应的命令处理函数
        it->second(clientfd, cmdMsg.substr(idx + 1));
    }
}

// 获取系统当前时间
string getCurrentTime()
{
    // time_t 和 time() 来自 <ctime>
    time_t now = time(0);
    // localtime() 和 tm 结构体来自 <ctime>
    tm *ltm = localtime(&now);
    if (!ltm)
    {
        return "Error: Failed to get local time";
    }
    // std::stringstream 来自 <sstream>
    std::stringstream ss;
    // std::put_time 来自 <iomanip>
    ss << std::put_time(ltm, "%Y-%m-%d %H:%M:%S");
    // std::string 来自 <string>
    return ss.str();
}
