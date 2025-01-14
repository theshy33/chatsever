#include "groupmodel.h"
#include "db.h"
#include <muduo/base/Logging.h>

//test
// #include <iostream>
// using namespace std;
//

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    // 1组装SQL语句
    char sql[1024] = {0};
    sprintf(sql,"insert into AllGroup(groupname,groupdesc)values('%s','%s')",
        group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
// 加入群组
bool GroupModel::joinGroup(int userId, int groupId, string role)
{
    // 先检查是否有该群组
    char sql[1024] = {0};
    if (int len=snprintf(sql,sizeof(sql),"select id from AllGroup where id = %d",groupId);len>=sizeof(sql))
    {
        LOG_ERROR << "sql is too long"<<'\n';
        return false;
    }
    sprintf(sql,"select id from AllGroup where id = %d", groupId);

    //cout<<"sql1:"<<sql<<endl;

    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)//查询成功
        {
            // 读取一行
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row != nullptr)//有该群组
            {
                mysql_free_result(res);//释放资源
                if(int len=snprintf(sql,sizeof(sql),"insert into GroupUser values(%d,%d,'%s')",
                    groupId,userId,role.c_str());len>=sizeof(sql))
                {
                    LOG_ERROR << "sql is too long"<<'\n';
                    return false;
                }
                if (mysql.update(sql))
                {
                    return true;
                }
            }
            else
            {
                mysql_free_result(res);//释放资源
            }
        }
    }
    return false;
}
// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userId)
{
    /*
    1.先根据用户id查询出所在的群组id列表
    2.根据群组id列表查询出所以用户信息
    */
    // 1组装SQL语句
    char sql[1024] = {0};
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from AllGroup a inner join \
        GroupUser b on a.id=b.groupid where b.userid=%d",
        userId);
    vector<Group> groupVec;

    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row{nullptr};
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.emplace_back(group);
            }
            mysql_free_result(res);
        }
    }
    // 2.根据群组id列表查询出所以用户信息
    for (auto &group : groupVec)
    {
        // User,GroupUser
        sprintf(sql,"select a.id,a.name,a.state,b.grouprole from User a \
            inner join GroupUser b on a.id = b.userid where b.groupid=%d",group.getId());
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row{nullptr};
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().emplace_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}
// 查询指定群组的用户id列表，除了自己，用于用户给给群组其他成员发消息
vector<int> GroupModel::queryGroupUsers(int userId, int groupId)
{
    // 1组装SQL语句
    char sql[1024] = {0};
    sprintf(sql,"select userid from GroupUser where groupid = %d and userid != %d",
        groupId, userId);
    vector<int> userIdVec;

    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row{nullptr};
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                userIdVec.emplace_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return move(userIdVec);//减少拷贝
}
// 退出群组
void GroupModel::exitGroup(int userId, int groupId)
{
    // 1组装SQL语句
    char sql[1024] = {0};
    sprintf(sql,"delete from GroupUser where userid = %d and groupid = %d",userId, groupId);

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}
