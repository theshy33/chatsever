#ifndef GROUPUSER_HPP
#define GROUPUSER_HPP

#include <string>
#include "user.hpp"

class GroupUser : public User 
{
public:
    void setRole(string role) { this->role = role; }
    string getRole() { return this->role; }
private:
    string role;
};
#endif // GROUPUSER_HPP
