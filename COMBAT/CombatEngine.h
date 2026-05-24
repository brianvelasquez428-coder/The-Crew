#pragma once
#include "Entity.h"
#include <vector>

std::vector<Entity*> getAITargets(Move selectedMove, std::vector<Entity*>& playerTeam);

// THE FIX: Added 'momentum' so the engine can pass it to the UI animations!
void executeCombatRoutine(Entity* attacker, Move selectedMove, std::vector<Entity*> targets, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, int& wallet, int momentum);
void executeEnemyTurn(Entity* character, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, int& wallet, int momentum);