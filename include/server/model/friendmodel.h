#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"
#include <vector>
using namespace std;

/// @brief 维护好友信息的操作接口方法
class FriendModel
{
public:
    //添加好友关系
    bool insert(int userid,int friendid);
    //返回用户的好友列表
    vector<User> query(int userid);
};

#endif // FRIENDMODEL_H
