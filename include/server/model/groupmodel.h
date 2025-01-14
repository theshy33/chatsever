#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <vector>
#include <string>
using namespace std;

class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group& group);
    // 加入群组
    bool joinGroup(int userId, int groupId, string role);
    // 查询用户所在群组信息
    vector<Group> queryGroups(int userId);
    // 查询指定群组的用户id列表，除了自己，用于用户给给群组其他成员发消息
    vector<int> queryGroupUsers(int userId, int groupId);
    // 退出群组
    void exitGroup(int userId, int groupId);
};

#endif // GROUPMODEL_H
