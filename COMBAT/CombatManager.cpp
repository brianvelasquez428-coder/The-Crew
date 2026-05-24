#include "raylib.h"
#include "CombatManager.h"
#include "CombatUI.h"
#include "CombatEngine.h"
#include "../Data/AbilityDatabase.h"
#include "../Data/MoveDatabase.h"
#include "../Systems/Logger.h"
#include <algorithm>
#include <string>

bool compareSpeed(Entity* a, Entity* b) { return a->currentSpeed > b->currentSpeed; }

void teachMove(Entity* character, MoveID moveID) {
    character->combatMenu.push_back(getMove(moveID));
    GameLog::Add(">>> [NEW SKILL] " + character->name + " learned " + getMoveName(moveID) + "!");
}

void startBattle(std::vector<Entity*>& masterRoster, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, std::vector<TeamUpSkill>& masterTeamUps, int inventory[3], int& wallet, bool canFlee) {
    
    // --- NEW: CINEMATIC INPUT BUFFER ---
    // Pauses for 0.3 seconds to swallow leftover key releases from the Hub
    double bootTime = GetTime();
    while (GetTime() - bootTime < 0.3 && !WindowShouldClose()) {
        BeginDrawing(); ClearBackground(BLACK); 
        DrawText("ENGAGING ENEMY...", 1920/2 - MeasureText("ENGAGING ENEMY...", 40)/2, 500, 40, RED); 
        EndDrawing();
    }
    // -----------------------------------
    GameLog::Clear(); GameLog::Add("BATTLE START!");
    std::vector<Entity*> combatants; std::vector<std::string> previouslyActiveTeamUps;
    bool battleIsActive = true; int roundCounter = 1; int teamMomentum = 0;       

    for (Entity* p : playerTeam) { p->calculateActiveStats(); applyStartOfBattlePassives(*p); }
    for (Entity* e : enemyTeam) { e->calculateActiveStats(); applyStartOfBattlePassives(*e); }

    while (battleIsActive && !WindowShouldClose()) {
        GameLog::Add("--- ROUND " + std::to_string(roundCounter) + " ---"); teamMomentum += 20; if (teamMomentum > 100) teamMomentum = 100;
        combatants.clear(); for (Entity* p : playerTeam) combatants.push_back(p); for (Entity* e : enemyTeam) combatants.push_back(e);
        std::sort(combatants.begin(), combatants.end(), compareSpeed);      

        for (Entity* character : combatants) {  
            if (WindowShouldClose()) break; // THE FIX: Abort the turn order if ESC was pressed!
            if (!character->isAlive || !battleIsActive) continue;
            bool isEnemy = std::find(enemyTeam.begin(), enemyTeam.end(), character) != enemyTeam.end();
            bool isPlayer = std::find(playerTeam.begin(), playerTeam.end(), character) != playerTeam.end();
            if (!isEnemy && !isPlayer) continue; 
            character->usedActiveAbilityThisTurn = false; 

            if (character->hasStatus(StatusType::Stun)) { handleStunnedCharacter(character, playerTeam, enemyTeam, teamMomentum); continue; }
            triggerPassives(*character); printCharacterStats(character); 

            if (!character->isBoss && isPlayer) executePlayerTurn(character, playerTeam, enemyTeam, masterRoster, masterTeamUps, inventory, wallet, teamMomentum, previouslyActiveTeamUps, battleIsActive, canFlee);
            else executeEnemyTurn(character, playerTeam, enemyTeam, wallet, teamMomentum); // THE FIX: Passed Momentum

            if (!battleIsActive) break;
            for (Entity* enemy : enemyTeam) if (enemy->isAlive) enemy->checkPhaseTransition();
            if (checkBattleEnd(playerTeam, enemyTeam, masterRoster)) { battleIsActive = false; break; }
            if (battleIsActive) pauseForPlayer(isPlayer, playerTeam, enemyTeam, teamMomentum);
        } 
        if (battleIsActive) { for (Entity* character : combatants) if (character->isAlive) character->endOfTurnUpdate(); }
        roundCounter++;
    }
    for (Entity* p : playerTeam) p->resetStats(); 
}