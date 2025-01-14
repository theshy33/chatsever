#include "friendmodel.h"
#include "db.h"
#include <muduo/base/Logging.h>
//添加好友关系
bool FriendModel::insert(int userid, int friendid)
{
    // 检查用户是否存在
    // 1组装SQL语句
    char sql[1024] = {0};
    if(int len = snprintf(sql,sizeof(sql),"select id from User where id = %d",friendid);
        len>=sizeof(sql))
    {
        LOG_ERROR<<"sql is too long"<<'\n';
        return false;
    }
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)//查询成功
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row != nullptr)//用户存在
            {
                // 用户存在
                mysql_free_result(res);
                if(int len = snprintf(sql,sizeof(sql),"insert into Friend values(%d,%d)",userid,friendid);
                    len>=sizeof(sql))
                {
                    LOG_ERROR<<"sql is too long"<<'\n';
                    return false;
                }
                if(mysql.update(sql))
                {
                    return true;
                }
            }
            else
            {
                mysql_free_result(res);
            }
        }
    }
    return false;
}
// 返回用户的好友列表
vector<User> FriendModel::query(int userid)
{
    // 1组装SQL语句
    char sql[1024] = {0};
    sprintf(sql,
    "select a.id,a.name,a.state from User a inner join Friend b on b.friendid = a.id where b.userid = %d",
    userid);

    vector<User> friends;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row {nullptr};
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                friends.emplace_back(user);
            }
            mysql_free_result(res);
        }
    }
    return move(friends);//右值引用，将vector转移到函数外
}
