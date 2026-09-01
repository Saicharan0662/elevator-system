#pragma once

#include <string>

// A person who interacts with the elevator system.
class User {
private:
    int userId;
    std::string name;

public:
    User(int userId, const std::string& name);

    // Getters
    int getUserId() const;
    std::string getName() const;

    // Setters
    void setUserId(int userId);
    void setName(const std::string& name);
};
