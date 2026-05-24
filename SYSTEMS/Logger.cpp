#include "Logger.h"

namespace GameLog {
    std::vector<std::string> messages;

    void Add(std::string message) {
        messages.push_back(message);
        if (messages.size() > 12) { 
            messages.erase(messages.begin()); // Delete oldest message
        }
    }

    std::vector<std::string> GetMessages() { return messages; }
    void Clear() { messages.clear(); }
}