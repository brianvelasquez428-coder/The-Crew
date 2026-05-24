#pragma once
#include <string>
#include <vector>

namespace GameLog {
    void Add(std::string message);
    std::vector<std::string> GetMessages();
    void Clear();
}