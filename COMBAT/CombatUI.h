#pragma once
#include "Entity.h"
#include "TeamUps.h"
#include <vector>
#include <string>

// --- THE NEW ANIMATION SYSTEM ---
void AnimateApproach(Entity* attacker, Entity* target, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum);
void AnimateReturn(Entity* attacker, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum);
void AnimateHit(Entity* target, int damage, bool isCrit, bool isDodge, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum, int durationFrames);
void AnimateDeath(Entity* target, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum);
void AnimateSupport(Entity* caster, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum);
// --------------------------------

void pauseForPlayer(bool wasPlayerTurn, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, int momentum);
void printCharacterStats(Entity* character);
void handleStunnedCharacter(Entity* character, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, int momentum);

std::vector<Entity*> requestPlayerTargets(Entity* attacker, Move selectedMove, std::vector<Entity*>& enemyTeam, std::vector<Entity*>& playerTeam);
void manageParty(std::vector<Entity*>& masterRoster, std::vector<Entity*>& activeParty);

void executePlayerTurn(Entity* character, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, std::vector<Entity*>& masterRoster, std::vector<TeamUpSkill>& masterTeamUps, int inventory[3], int& wallet, int& teamMomentum, std::vector<std::string>& previouslyActiveTeamUps, bool& battleIsActive, bool canFlee);

bool checkBattleEnd(std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, std::vector<Entity*>& masterRoster);