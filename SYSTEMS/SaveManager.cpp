#include "SaveManager.h"
#include <fstream>
#include <cstdio>
#include <string>

namespace SaveSystem {
    const std::string SAVE_FILE = "savegame.dat";

    bool DoesSaveExist() {
        std::ifstream file(SAVE_FILE);
        return file.good();
    }

    void SaveGame(int wallet, int inventory[3]) {
        std::ofstream file(SAVE_FILE);
        if (file.is_open()) {
            file << wallet << "\n";
            file << inventory[0] << "\n" << inventory[1] << "\n" << inventory[2] << "\n";
            file.close();
        }
    }

    bool LoadGame(int& wallet, int inventory[3]) {
        std::ifstream file(SAVE_FILE);
        if (file.is_open()) {
            file >> wallet;
            file >> inventory[0]; 
            file >> inventory[1]; 
            file >> inventory[2];
            file.close();
            return true;
        }
        return false;
    }

    void DeleteSave() {
        std::remove(SAVE_FILE.c_str());
    }
}