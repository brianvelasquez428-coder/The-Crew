#pragma once
#include "Entity.h"
#include "TeamUps.h"
#include <vector>

void startBattle(std::vector<Entity*>& masterRoster, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, std::vector<TeamUpSkill>& masterTeamUps, int inventory[3], int& wallet, bool canFlee = true);
void teachMove(Entity* character, MoveID moveID); // Updated signature