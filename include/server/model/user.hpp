#ifndef USER_HPP
#define USER_HPP
#include <string>
using namespace std;

/// @brief User表的ORM类
/// @note 该类用于管理用户信息，包括id、用户名、密码、状态等信息
class User
{
public:
    User(int id=-1, string name="", string password="", string state="offline")
    : id(id), name(name), password(password), state(state)
    {}

    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setPassword(string password) { this->password = password; }
    void setState(string state) { this->state = state; }

    int getId() { return id; }
    string getName() { return name; }    
    string getPassword() { return password; }
    string getState() { return state; }
private:
    int id;
    string name;
    string password;
    string state;
};

#endif // USER_HPP
