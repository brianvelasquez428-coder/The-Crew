#include "CombatEngine.h"
#include "CombatUI.h" 
#include "../Data/AbilityDatabase.h"
#include "../Systems/Logger.h"
#include <algorithm>
#include <string>

std::vector<Entity*> getAITargets(Move selectedMove, std::vector<Entity*>& playerTeam) {
    std::vector<Entity*> bossTargets;            
    if (selectedMove.target == MoveTarget::AllEnemies) {
        for (Entity* target : playerTeam) if (target->isAlive) bossTargets.push_back(target);
    } else {
        for (Entity* target : playerTeam) {
            if (target->isAlive && target->hasStatus(StatusType::Taunt)) { bossTargets.push_back(target); break; }
        }
        int targetsNeeded = (selectedMove.target == MoveTarget::TwoEnemies) ? 2 : 1;
        while (bossTargets.size() < targetsNeeded) {
            std::vector<Entity*> validTargets;
            for (Entity* target : playerTeam) {
                if (target->isAlive && std::find(bossTargets.begin(), bossTargets.end(), target) == bossTargets.end()) {
                    validTargets.push_back(target);
                }
            }
            if (validTargets.empty()) break; 
            int randomIndex = rand() % validTargets.size();
            bossTargets.push_back(validTargets[randomIndex]);
        }
    }
    return bossTargets;
}

void executeEnemyTurn(Entity* character, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, int& wallet, int momentum) {
    std::vector<Move> affordableMoves;
    for (int i = 0; i < character->combatMenu.size(); i++) {
        if (character->currentStamina >= character->combatMenu[i].staminaCost) affordableMoves.push_back(character->combatMenu[i]);
    }
    if (affordableMoves.empty()) affordableMoves.push_back(character->combatMenu[0]); 

    Move selectedMove = affordableMoves[rand() % affordableMoves.size()];
    character->currentStamina -= selectedMove.staminaCost;
    GameLog::Add(character->name + " uses " + selectedMove.name + "!");
    
    std::vector<Entity*> bossTargets = getAITargets(selectedMove, playerTeam);
    executeCombatRoutine(character, selectedMove, bossTargets, playerTeam, enemyTeam, wallet, momentum);
}

void executeCombatRoutine(Entity* attacker, Move selectedMove, std::vector<Entity*> targets, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, int& wallet, int momentum) {
    if (targets.empty()) return;
    int hitsLanded = selectedMove.hitCount;

    if (attacker->isBoss && attacker->hiddenAbility == PassiveID::Combo) {
        int extraHits = (rand() % 3) + 1; hitsLanded += extraHits;
        GameLog::Add("[Combo] " + attacker->name + " adds " + std::to_string(extraHits) + " extra strikes!");
    }

    if (hitsLanded > 0 && targets.size() > 1) GameLog::Add(attacker->name + " unleashes an Area Attack!");
    else if (hitsLanded > 0) GameLog::Add(attacker->name + " unleashes a " + std::to_string(hitsLanded) + "-hit combo!");

    int rawTotalDamage = attacker->currentAttack + attacker->tempDamageBonus; 
    int totalDamagePool = rawTotalDamage * selectedMove.powerMultiplier; 

    if (selectedMove.effect == Effect::StrikeUlt) {
        int boost = std::min(120, attacker->screwDatStacks * 20); 
        totalDamagePool += (totalDamagePool * boost) / 100;
        attacker->screwDatStacks = 0; 
        GameLog::Add("[Ult Boost] Damage increased by " + std::to_string(boost) + "%!");
    }

    attacker->tempDamageBonus = 0; 
    int baseDamagePerHit = totalDamagePool; 
    bool isSupport = (selectedMove.target == MoveTarget::Self || selectedMove.target == MoveTarget::OneAlly || selectedMove.target == MoveTarget::TwoAllies || selectedMove.target == MoveTarget::AllAllies);
    
    for (Entity* target : targets) {
        if (!isSupport) AnimateApproach(attacker, target, playerTeam, enemyTeam, momentum);
        else if (isSupport && target == targets[0]) AnimateSupport(attacker, playerTeam, enemyTeam, momentum);
        
        executeMoveEffect(selectedMove.effect, *attacker, *target);
                 
        if (hitsLanded > 0 && !isSupport) {
            for (int h = 1; h <= hitsLanded; h++) {
                if (!target->isAlive) { GameLog::Add(target->name + " is down!"); break; }
                int defenderEvasionStat = (target->name == "Brian" && !target->isAltStance) ? target->currentSIQ : target->currentBIQ; 
                int biqDifference = attacker->currentBIQ - defenderEvasionStat; 
                int hitChance = 85 + (biqDifference / 2); 
                if (hitChance > 100) hitChance = 100; if (hitChance < 20) hitChance = 20;
                bool isHit = (rand() % 100) < hitChance;
                if (selectedMove.effect == Effect::StrikeUlt) isHit = true; 
                                  
                int hitDelayFrames = 15 + (rand() % 25); 
                if (!isHit) { 
                     GameLog::Add("* WHOOSH! " + target->name + " dodged! *"); 
                     triggerOnDodge(*target); 
                     AnimateHit(target, 0, false, true, playerTeam, enemyTeam, momentum, hitDelayFrames); 
                     continue; 
                 }
                if (target->hitNullificationStacks > 0) { target->hitNullificationStacks--; GameLog::Add("* CLANG! " + target->name + " nullified the hit! *"); continue; }
                int critChance = std::max(1, 5 + (biqDifference / 10)); bool isCrit = (rand() % 100) < critChance;
                int rawHitDamage = std::max(1, baseDamagePerHit + ((rand() % 5) - 2));
                bool wasAlive = target->isAlive;
                int hpBefore = target->currentHP; target->takeDamage(rawHitDamage, isCrit); int actualDamage = hpBefore - target->currentHP;
                if (isCrit) GameLog::Add("CRITICAL HIT! " + target->name + " takes " + std::to_string(actualDamage) + " damage!"); else GameLog::Add(target->name + " takes " + std::to_string(actualDamage) + " damage.");
                AnimateHit(target, actualDamage, isCrit, false, playerTeam, enemyTeam, momentum, hitDelayFrames);
                if (wasAlive && !target->isAlive) {
                    GameLog::Add(target->name + " has been defeated!");
                    AnimateDeath(target, playerTeam, enemyTeam, momentum); 
                    bool isEnemy = std::find(enemyTeam.begin(), enemyTeam.end(), target) != enemyTeam.end();
                    if (isEnemy) {
                        GameLog::Add("[LOOT] Got " + std::to_string(target->expDropValue) + " EXP & $" + std::to_string(target->moneyDropValue) + "!"); wallet += target->moneyDropValue; 
                        for (Entity* player : playerTeam) if (player->isAlive) player->gainEXP(target->expDropValue);
                    }
                }
            }
        } else if (isSupport) {
            AnimateHit(target, -1, false, false, playerTeam, enemyTeam, momentum, 30);
        }
    }
    
    // ---> FIX: Only Animate Return once all targets have been fully processed
    if (!isSupport) {
        AnimateReturn(attacker, playerTeam, enemyTeam, momentum);
    }
}