#include "User.h"

User::User(int userId, const std::string& name)
    : userId(userId), name(name) {}

int User::getUserId() const {
    return userId;
}

std::string User::getName() const {
    return name;
}

void User::setUserId(int userId) {
    this->userId = userId;
}

void User::setName(const std::string& name) {
    this->name = name;
}
