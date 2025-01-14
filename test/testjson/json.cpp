#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

//json实例化1
string func1()
{
    json js;
    // 添加数组
    js["id"] = {1,2,3,4,5};
    // 添加key-value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello china"}};
    // cout << js << endl;
    // string sendBuf = js.dump();//取出字符信息
    // cout<<sendBuf<<endl;
    // cout<<sendBuf.c_str()<<endl;
    // return sendBuf;
    return js.dump();
}

void func2()
{
    vector<int> vec;
    for(int i=0;i<3;++i)
        vec.emplace_back(i);
    map<int,string> mp;
    mp[1]="hello";
    mp.insert({3,"ss"});
    json js;
    js["list"]=vec;
    js["map"]=mp;
    cout<<js<<endl;
    auto v=js["list"];
    map<int,string> m=js["map"];
    for(auto &i:v)
        cout<<i<<' ';
    cout<<endl;
    for(auto& p:m)
        cout<<p.first<<" "<<p.second<<endl;
}
int main()
{
    //func1();
    //func2();
    string recvBuf = func1();
    //数据反序列化
    json jsBuf = json::parse(recvBuf);
    cout<<jsBuf["id"]<<endl;
    auto arr = jsBuf["id"];
    cout<<arr[2]<<endl;
    auto msgJs = jsBuf["msg"];
    cout<<msgJs["zhang san"]<<endl;
    cout<<msgJs["liu shuo"]<<endl;
    return 0;
}

