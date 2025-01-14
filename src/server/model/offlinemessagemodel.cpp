#include "offlinemessagemodel.h"
#include "db.h"


// 存储用户的离线消息

void OfflineMsgModel::insert(int userid, string msg)
{
    // 1组装SQL语句
    char sql[1024] = {0};
    sprintf(sql,"insert into OfflineMessage values('%d','%s')",userid,msg.c_str());

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

// 删除用户的离线消息
void OfflineMsgModel::remove(int userid)
{
    // 1组装SQL语句
    char sql[1024] = {0};
    sprintf(sql,"delete from OfflineMessage where userid = '%d'",userid);

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户的离线消息
vector<string> OfflineMsgModel::query(int userid)
{
    // 1组装SQL语句
    char sql[1024] = {0};
    sprintf(sql,"select message from OfflineMessage where userid = %d",userid);

    vector<string> msglist;//vec
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row {nullptr};
            //把userid用户所有离线消息放入msgList
            while((row = mysql_fetch_row(res)) != nullptr)//不断获取结果集
            {
                msglist.emplace_back(row[0]);
            }
            mysql_free_result(res);//释放mysql资源
            //return msglist;
        }
    }
    return move(msglist);//右值引用，避免拷贝，提高效率
}
