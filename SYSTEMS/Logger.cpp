#include "Logger.h"

namespace GameLog {
    std::vector<std::string> messages;

    void Add(std::string message) {
        messages.push_back(message);
        if (messages.size() > 100) { // <--- Increased from 12 to 100
            messages.erase(messages.begin()); 
        }
    }

    std::vector<std::string> GetMessages() { return messages; }
    void Clear() { messages.clear(); }
}