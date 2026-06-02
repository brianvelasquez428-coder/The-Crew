#include "Logger.h"

namespace GameLog {
    std::vector<std::string> messages;
    int totalLogged = 0; 

    void Add(const std::string& message) {
        int maxChars = 55; // Safe character limit for your 720px wide Combat Log
        std::string currentLine = "";
        std::string word = "";

        // Auto-Wrap Engine
        for (char c : message) {
            if (c == ' ') {
                if (currentLine.length() + word.length() > maxChars) {
                    messages.push_back(currentLine);
                    totalLogged++;
                    // Indent the wrapped line slightly so it looks grouped together!
                    currentLine = "  " + word + " "; 
                } else {
                    currentLine += word + " ";
                }
                word = "";
            } else {
                word += c;
            }
        }
        
        // Push the final leftover word(s)
        if (currentLine.length() + word.length() > maxChars) {
            messages.push_back(currentLine);
            totalLogged++;
            messages.push_back("  " + word);
            totalLogged++;
        } else {
            messages.push_back(currentLine + word);
            totalLogged++;
        }

        // Keep the log capped at 100 lines to save memory
        while (messages.size() > 100) { 
            messages.erase(messages.begin()); 
        }
    }

    std::vector<std::string> GetMessages() { return messages; }
    void Clear() { messages.clear(); totalLogged = 0; }
    int GetTotalMessagesLogged() { return totalLogged; }
}