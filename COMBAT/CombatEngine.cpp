#include "CombatEngine.h"
#include "CombatUI.h" 
#include "../Data/AbilityDatabase.h"
#include "../Systems/Logger.h"
#include <algorithm>
#include <string>

std::vector<Entity*> getAITargets(const Move& selectedMove, std::vector<Entity*>& playerTeam) {
    std::vector<Entity*> bossTargets;  
    bossTargets.reserve(playerTeam.size()); // <--- NEW: Prevents mid-calculation resizing
              
    if (selectedMove.target == MoveTarget::AllEnemies) {
        for (Entity* target : playerTeam) if (target->isAlive) bossTargets.push_back(target);
    } else {
        for (Entity* target : playerTeam) {
            // Make sure it says hasStatusID(StatusID::Taunting)
            if (target->isAlive && target->hasStatusID(StatusID::Taunting)) { bossTargets.push_back(target); break; }
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
    
    // --> THE FIX: Safely subtract stamina without going into debt
    if (character->currentStamina >= selectedMove.staminaCost) {
        character->currentStamina -= selectedMove.staminaCost;
    } else {
        character->currentStamina = 0; 
    }
    
    GameLog::Add(character->name + " uses " + selectedMove.name + "!");
    
    // ---> FIX: Prevent AI from targeting players with Self/Ally Support moves! <---
    std::vector<Entity*> bossTargets;
    if (selectedMove.target == MoveTarget::Self) {
        bossTargets.push_back(character);
    } else if (selectedMove.target == MoveTarget::AllAllies || selectedMove.target == MoveTarget::OneAlly || selectedMove.target == MoveTarget::TwoAllies) {
        for (Entity* e : enemyTeam) if (e->isAlive) bossTargets.push_back(e);
    } else {
        bossTargets = getAITargets(selectedMove, playerTeam);
    }

    executeCombatRoutine(character, selectedMove, bossTargets, playerTeam, enemyTeam, wallet, momentum);
}

void executeCombatRoutine(Entity* attacker, const Move& selectedMove, std::vector<Entity*> targets, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, int& wallet, int momentum) {
    if (targets.empty()) return;
    int hitsLanded = selectedMove.hitCount;

    if (attacker->isBoss && attacker->hiddenAbility == PassiveID::Combo) {
        int chance = rand() % 100;
        int extraHits = 0;
        
        if (chance < 45) extraHits = 0;      
        else if (chance < 80) extraHits = 1; 
        else extraHits = 2;                  
        
        if (extraHits > 0) {
            hitsLanded += extraHits;
            GameLog::Add("[Combo] " + attacker->name + " adds " + std::to_string(extraHits) + " extra strikes!");
        }
    }

    if (hitsLanded > 0 && targets.size() > 1) GameLog::Add(attacker->name + " unleashes an Area Attack!");
    else if (hitsLanded > 0) GameLog::Add(attacker->name + " unleashes a " + std::to_string(hitsLanded) + "-hit combo!");

    int rawTotalDamage = attacker->currentAttack + attacker->tempDamageBonus; 
    
    // ---> THE FIX: Apply Defense Scaling natively before the math is finalized! <---
    if (selectedMove.effect == Effect::DefenseScalingDamage) {
        rawTotalDamage += attacker->currentDefense;
    }
    
    int totalDamagePool = rawTotalDamage * selectedMove.powerMultiplier; 

    // --- UNIVERSAL ULTIMATE BOOST ---
    if (selectedMove.category == MoveCategory::Ultimate && attacker->naturalAbility == NaturalID::ScrewDat) {
        int boost = std::min(120, attacker->screwDatStacks * 30); 
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
        
        // ---> 1. ON-CAST EFFECTS <---
        // Happens immediately, even if the attack is going to miss!
        if (selectedMove.effectTiming == EffectTiming::OnCast) {
            executeMoveEffect(selectedMove.effect, *attacker, *target);
            AnimatePopupsForEntity(attacker, playerTeam, enemyTeam, momentum);
            AnimatePopupsForEntity(target, playerTeam, enemyTeam, momentum);
        }
                 
        if (hitsLanded > 0 && !isSupport) {
            // Track if we've already applied hit effects so multi-hit moves don't trigger 10 times!
            bool preHitApplied = false;
            bool postHitApplied = false;

            for (int h = 1; h <= hitsLanded; h++) {
                if (!target->isAlive) { GameLog::Add(target->name + " is down!"); break; }
                int defenderEvasionStat = (target->actorID == ActorID::Brian && !target->isAltStance) ? target->currentSIQ : target->currentBIQ;
                int biqDifference = attacker->currentBIQ - defenderEvasionStat; 
                int hitChance = 85 + (biqDifference / 2); 
                if (hitChance > 100) hitChance = 100; if (hitChance < 20) hitChance = 20;
                bool isHit = (rand() % 100) < hitChance;
                
                if (selectedMove.ignoresEvasion) isHit = true; 
                                  
                int hitDelayFrames = 15 + (rand() % 25);
                if (!isHit) { 
                     GameLog::Add("* WHOOSH! " + target->name + " dodged! *"); 
                     triggerOnDodge(*target); 
                     AnimateHit(target, 0, false, true, playerTeam, enemyTeam, momentum, hitDelayFrames); 
                     continue; 
                 }
                 
                if (target->hitNullificationStacks > 0) { 
                    target->hitNullificationStacks--; 
                    GameLog::Add("* CLANG! " + target->name + " nullified the hit! *"); 
                    target->pendingPopups.push_back({"NULLIFIED!", true});
                    AnimatePopupsForEntity(target, playerTeam, enemyTeam, momentum);
                    continue; 
                }
                
                // ---> 2. PRE-HIT EFFECTS <---
                // Attack connected! Apply effect before math, but only ONCE per target!
                if (selectedMove.effectTiming == EffectTiming::PreHit && !preHitApplied) {
                    executeMoveEffect(selectedMove.effect, *attacker, *target);
                    AnimatePopupsForEntity(attacker, playerTeam, enemyTeam, momentum);
                    AnimatePopupsForEntity(target, playerTeam, enemyTeam, momentum);
                    preHitApplied = true;
                }

                int critChance = std::max(1, 5 + (biqDifference / 10)); bool isCrit = (rand() % 100) < critChance;
                int rawHitDamage = std::max(1, baseDamagePerHit);

                if (attacker->naturalAbility == NaturalID::Sparring) {
                    float comboBonus = 0.05f * (h - 1); 
                    if (comboBonus > 0) {
                        rawHitDamage += (int)(rawHitDamage * comboBonus);
                        GameLog::Add("[SPARRING] Combo chain! Damage +" + std::to_string((int)(comboBonus * 100)) + "%!");
                    }
                }
                
                int originalDef = target->currentDefense;
                
                if (selectedMove.effect == Effect::IgnoreDefense) {
                    target->currentDefense /= 2; 
                }

                bool wasAlive = target->isAlive;
                int actualDamage = target->takeDamage(rawHitDamage, isCrit); 
                
                if (selectedMove.effect == Effect::IgnoreDefense) {
                    target->currentDefense = originalDef;
                }

                if (isCrit) GameLog::Add("CRITICAL HIT! " + target->name + " takes " + std::to_string(actualDamage) + " damage!"); else GameLog::Add(target->name + " takes " + std::to_string(actualDamage) + " damage.");
                AnimateHit(target, actualDamage, isCrit, false, playerTeam, enemyTeam, momentum, hitDelayFrames);
                
                // ---> THE FIX: Animate the death immediately so the sprite fades cleanly <---
                if (wasAlive && !target->isAlive) {
                    GameLog::Add(target->name + " has been defeated!");
                    AnimateDeath(target, playerTeam, enemyTeam, momentum); 
                    bool isEnemy = std::find(enemyTeam.begin(), enemyTeam.end(), target) != enemyTeam.end();
                    if (isEnemy) {
                        GameLog::Add("[LOOT] Got " + std::to_string(target->expDropValue) + " EXP & $" + std::to_string(target->moneyDropValue) + "!"); wallet += target->moneyDropValue; 
                        for (Entity* player : playerTeam) if (player->isAlive) player->gainEXP(target->expDropValue);
                    }
                }

                // ---> 3. POST-HIT EFFECTS <---
                if (selectedMove.effectTiming == EffectTiming::PostHit && !postHitApplied) {
                    executeMoveEffect(selectedMove.effect, *attacker, *target);
                    AnimatePopupsForEntity(attacker, playerTeam, enemyTeam, momentum);
                    
                    // Only draw target popups if they survived the hit!
                    if (target->isAlive) {
                        AnimatePopupsForEntity(target, playerTeam, enemyTeam, momentum);
                    }
                    postHitApplied = true;
                }
                
                // --- THE FIX: Break the loop AFTER post-hit runs for the attacker ---
                if (!target->isAlive) break; 

                AnimatePopupsForEntity(target, playerTeam, enemyTeam, momentum);

                if (target->checkPhaseTransition()) {
                    AnimatePhaseTransition(target->currentPhase, playerTeam, enemyTeam, momentum);
                }
            }
        } else if (isSupport) {
            AnimateHit(target, -1, false, false, playerTeam, enemyTeam, momentum, 30);
        }
    }
    
    if (!isSupport) {
        AnimateReturn(attacker, playerTeam, enemyTeam, momentum);
    }

    for (Entity* p : playerTeam) {
        if (p->isAlive && p->naturalAbility == NaturalID::Mediator && !p->mediatorAwakened) {
            for (Entity* ally : playerTeam) {
                if (ally != p && (!ally->isAlive || ((float)ally->currentHP / ally->maxHP) <= 0.20f)) {
                    p->mediatorAwakened = true;
                    GameLog::Add("[MEDIATOR] " + p->name + " refuses to let his friends fall!");
                    GameLog::Add("           (Damage Reduction Awakened to 30%!)");
                    break;
                }
            }
        }
    }
}