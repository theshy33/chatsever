#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

/// @brief User表的数据操作类
class UserModel
{
public:
    // User表的增加操作
    bool insertUser(User& user);
    // 根据id查询User表的操作
    User queryUser(int id);
    // 更新用户的状态信息
    bool updateUserState(User& user);
    // 重置用户状态信息
    void resetState();
private:
};

#endif // USERMODEL_H
