#pragma once

namespace SaveSystem {
    bool DoesSaveExist();
    void SaveGame(int wallet, int inventory[3]);
    bool LoadGame(int& wallet, int inventory[3]);
    void DeleteSave();
}