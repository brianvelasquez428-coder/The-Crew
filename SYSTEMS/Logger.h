#pragma once 
#include <string> 
#include <vector> 

namespace GameLog {
    void Add(const std::string& message);
    std::vector<std::string> GetMessages();
    void Clear();
    
    // ---> THE FIX: Add this new function <---
    int GetTotalMessagesLogged(); 
}