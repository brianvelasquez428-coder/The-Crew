#pragma once
#include "Entity.h"
#include <vector>

std::vector<Entity*> getAITargets(const Move& selectedMove, std::vector<Entity*>& playerTeam);
void executeCombatRoutine(Entity* attacker, const Move& selectedMove, std::vector<Entity*> targets, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, int& wallet, int momentum);
void executeEnemyTurn(Entity* character, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, int& wallet, int momentum);