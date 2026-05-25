#include "Entity.h"
#include <iostream>
#include <algorithm>

std::string getStatusName(StatusID id) {
    switch(id) {
        case StatusID::TauntedATK: return "Taunted ATK";
        case StatusID::TauntedDEF: return "Taunted DEF";
        case StatusID::Taunting: return "Taunting";
        case StatusID::DeepEndSIQ: return "Deep End SIQ";
        case StatusID::DeepEndBIQ: return "Deep End BIQ";
        case StatusID::Bleed: return "Bleed";
        case StatusID::GuardBreak: return "Guard Break";
        case StatusID::Hardened: return "Hardened";
        case StatusID::Slowed: return "Slowed";
        case StatusID::GuardBroken: return "Guard Broken";
        case StatusID::BoggedDown: return "Bogged Down";
        case StatusID::PrecisionDEFDrop: return "Precision DEF Drop";
        case StatusID::PrecisionBIQDrop: return "Precision BIQ Drop";
        case StatusID::WindKnockedOut: return "Wind Knocked Out";
        case StatusID::Stunned: return "Stunned";
        case StatusID::DemolitionistATK: return "Demolitionist ATK";
        case StatusID::DemolitionistBIQ: return "Demolitionist BIQ";
        case StatusID::CrashOutATK: return "Crash Out ATK";
        case StatusID::CrashOutDEF: return "Crash Out DEF";
        case StatusID::UnstoppableATK: return "Unstoppable ATK";
        case StatusID::UnstoppableSPD: return "Unstoppable SPD";
        case StatusID::LightFeet: return "Light Feet";
        case StatusID::HeavyHands: return "Heavy Hands";
        case StatusID::IronWill: return "Iron Will";
        default: return "Unknown Status";
    }
}

Entity::Entity(std::string spawnName, LifeStage spawnStage, bool spawnIsBoss) {
    name = spawnName; 
    internalID = EntityID::Unknown; 
    actorID = ActorID::Enemy;       
    currentStage = spawnStage; currentPhase = 1; isBoss = spawnIsBoss;
    
    // ---> FIX: Use nullptr instead of "None"
    grabbedBy = nullptr; isGrabbed = false; shieldHP = 0; 
    
    usedActiveNatural = false; usedActiveAbilityThisTurn = false; usedActives.clear(); isAltStance = false;           
    tempDamageBonus = 0; hitNullificationStacks = 0; screwDatStacks = 0; screwDatDecayTimer = 0;
    currentHP = maxHP; isAlive = true;      
    currentAttack = baseAttack; currentDefense = baseDefense; currentSpeed = baseSpeed; currentStamina = maxStamina;
    level = 1; currentEXP = 0; expToNextLevel = 100; expDropValue = 0; moneyDropValue = 0; 
}

void Entity::addStatus(StatusEffect s) {
    if (!s.isStackable) {
        for (auto& existing : activeStatuses) {
            if (existing.id == s.id) {
                existing.duration = s.duration; 
                existing.value = s.value;       
                calculateActiveStats();
                return;
            }
        }
    }
    
    // --- NEW: Simplified Popup Text Logic ---
    bool isBuff = true;
    if (s.type == StatusType::StatModifier && s.value < 0) isBuff = false;
    if (s.type == StatusType::Bleed || s.type == StatusType::Stun || s.type == StatusType::StaminaPenalty || s.type == StatusType::Taunt) isBuff = false;

    std::string pText = "";
    if (s.type == StatusType::StatModifier) {
        std::string statName = "";
        if (s.targetStat == StatName::ATK) statName = "ATK";
        else if (s.targetStat == StatName::DEF) statName = "DEF";
        else if (s.targetStat == StatName::SPD) statName = "SPD";
        else if (s.targetStat == StatName::BIQ) statName = "BIQ";
        else if (s.targetStat == StatName::SIQ) statName = "SIQ";

        if (s.value > 0) pText = statName + " UP";
        else pText = statName + " DOWN";
    } 
    else if (s.type == StatusType::Bleed) pText = "BLEED";
    else if (s.type == StatusType::Stun) pText = "STUN";
    else if (s.type == StatusType::Taunt) pText = "TAUNT";
    else if (s.type == StatusType::StaminaPenalty) pText = "STAMINA DOWN"; // <--- FIXED THIS!
    else {
        // Fallback: If any other custom statuses slip through, we force them to be short here
        std::string rawName = getStatusName(s.id);
        if (rawName == "Wind Knocked Out") pText = "STAMINA DOWN";
        else pText = rawName; 
    }

    pendingPopups.push_back({pText, isBuff});

    activeStatuses.push_back(s);
    calculateActiveStats();
}

bool Entity::hasStatus(StatusType type) {
    for (auto& s : activeStatuses) if (s.type == type) return true;
    return false;
}

bool Entity::hasStatusID(StatusID id) {
    for (auto& s : activeStatuses) if (s.id == id) return true;
    return false;
}

void Entity::removeStatusByType(StatusType type) {
    activeStatuses.erase(std::remove_if(activeStatuses.begin(), activeStatuses.end(),
        [type](const StatusEffect& s) { return s.type == type; }), activeStatuses.end());
    calculateActiveStats();
}

// In CORE/Entity.cpp (Around line 70)

int Entity::takeDamage(int rawDamage, bool isCritical) {
    if (shieldHP > 0 && isCriticalOnlyShield == true) {
        if (!isCritical) { 
            std::cout << name << "'s filter absorbed the attack! (0 Damage)\n"; 
            return 0; // <--- Returns 0 if blocked
        }
        else std::cout << "Critical Strike Pierces though the filter!\n";
    }
         
    float defenseMultiplier = 100.0f / (100.0f + currentDefense);       
    float calculatedDamage = rawDamage * defenseMultiplier;                    
    
    if (isCritical) {
        calculatedDamage *= 2.0f;
    }
    
    float variance = 1.0f + (((rand() % 21) - 10) / 100.0f);
    calculatedDamage *= variance;
    
    int finalDamage = std::max(1, (int)calculatedDamage);
    
    // ... [The rest of your shield / HP deduction logic stays exactly the same from here down]
    if (shieldHP > 0) {
        int damageToShield = std::min(shieldHP, finalDamage);
        shieldHP -= damageToShield;
        int spilloverDamage = finalDamage - damageToShield;
        if (shieldHP <= 0) {
            shieldHP = 0; isCriticalOnlyShield = false; 
            if (isCritical) std::cout << "   [CRIT!] "; else std::cout << "   ";
            std::cout << "Massive hit for " << finalDamage << " total damage!\n";
            std::cout << "   " << name << "'s shield absorbs " << damageToShield << " before SHATTERING!\n";
            if (spilloverDamage > 0) {
                currentHP -= spilloverDamage; if (currentHP < 0) currentHP = 0;
                std::cout << "   " << spilloverDamage << " damage spills over directly to " << name << "! (" << currentHP << " HP remaining)\n";
            }
        } else {
            if (isCritical) std::cout << "   [CRIT!] "; else std::cout << "   ";
            std::cout << name << "'s shield absorbs the full " << finalDamage << " damage! (" << shieldHP << " Shield HP remaining)\n";
        }
    } else {
        currentHP -= finalDamage; if (currentHP < 0) currentHP = 0; 
        if (isCritical) std::cout << "   [CRIT!] " << name << " takes " << finalDamage << " damage! (" << currentHP << " HP remaining)\n";
        else std::cout << "   " << name << " takes " << finalDamage << " damage! (" << currentHP << " HP remaining)\n";
    }
    if (currentHP <= 0) { currentHP = 0; isAlive = false; std::cout << "\n>>> " << name << " has been knocked out! <<<\n"; }

    return finalDamage;
}

bool Entity::useStamina(int cost) {
    if (currentStamina >= cost) { currentStamina -= cost; return true; }
    else { std::cout << name << " doesn't have enough stamina for that!\n"; return false; }
}

bool Entity::checkPhaseTransition() {
    if (!isBoss || extraPhases.empty()) return false; 
    if (currentHP <= extraPhases[0].thresholdHP) { 
        std::cout << "\n=======================================================\n";
        std::cout << extraPhases[0].transitionText << "\n";
        std::cout << "=======================================================\n\n";
        baseAttack = extraPhases[0].newAttack; baseDefense = extraPhases[0].newDefense; baseSpeed = extraPhases[0].newSpeed;
        baseBIQ = extraPhases[0].newBIQ; baseSIQ = extraPhases[0].newSIQ; maxStamina = baseSIQ * 2; 
        
        naturalAbility = extraPhases[0].newNaturalAbility; passiveAbilities = extraPhases[0].newPassiveAbilities; 
        activeAbilities = extraPhases[0].newActiveAbilities; combatMenu = extraPhases[0].newCombatMenu;
        currentPhase++; extraPhases.erase(extraPhases.begin());                     
        calculateActiveStats(); 
        
        return true; // <--- NEW: Tells the engine a phase shift occurred
    }
    return false;
}

bool Entity::executeTeamUp(Entity& partner, int staminaCost) {
    if (currentStamina >= staminaCost && partner.currentStamina >= staminaCost) {
        currentStamina -= staminaCost; partner.currentStamina -= staminaCost;
        std::cout << name << " and " << partner.name << " successfully initiate a Team-Up!\n"; return true; 
    } else { std::cout << "Team-Up failed! Someone is out of breath.\n"; return false; }
}

bool Entity::executeTeamUp(Entity& partner1, Entity& partner2, int staminaCost) {
    if (currentStamina >= staminaCost && partner1.currentStamina >= staminaCost && partner2.currentStamina >= staminaCost) {
        currentStamina -= staminaCost; partner1.currentStamina -= staminaCost; partner2.currentStamina -= staminaCost;
        std::cout << name << ", " << partner1.name << ", and " << partner2.name << " initiate a massive Team-Up!\n"; return true; 
    } else { std::cout << "Team-Up failed! The crew doesn't have the stamina.\n"; return false; }
}

void Entity::healHP(int amount) {
    if (!isAlive) { std::cout << name << " is knocked out and cannot be healed this way!\n"; return; }
    currentHP = std::min(maxHP, currentHP + amount);
    std::cout << name << " recovers HP! (Current HP: " << currentHP << "/" << maxHP << ")\n";
}

void Entity::endOfTurnUpdate() {
    for (auto it = activeStatuses.begin(); it != activeStatuses.end(); ) {
        if (it->type == StatusType::Bleed) {
            std::cout << "\n   [BLEED] " << name << " loses " << it->value << " HP from the open wound!\n";
            currentHP -= it->value;
            if (currentHP <= 0) { currentHP = 0; isAlive = false; std::cout << ">>> " << name << " succumbed to their wounds! <<<\n"; }
        }
        it->duration--;
        if (it->duration <= 0) {
            if (it->type == StatusType::Stun) std::cout << name << " shook off the stun and is ready to fight!\n";
            else if (it->type == StatusType::Taunt) std::cout << name << " is no longer drawing attacks.\n";
            else if (it->type == StatusType::StaminaPenalty) std::cout << name << " catches their breath! (Stamina Regen restored)\n";
            else std::cout << name << "'s [" << getStatusName(it->id) << "] wore off.\n";
            it = activeStatuses.erase(it); 
        } else {
            ++it;
        }
    }
    
    if (screwDatDecayTimer > 0) {
        screwDatDecayTimer--;
        if (screwDatDecayTimer <= 0) { screwDatStacks = 0; std::cout << "   [FADED] " << name << "'s Strike Ultimate momentum has completely vanished!\n"; }
    }
         
    calculateActiveStats();
    regenerateStamina(); 
}

void Entity::calculateActiveStats() {
    currentAttack = baseAttack; currentDefense = baseDefense; currentSpeed = baseSpeed;
    currentBIQ = baseBIQ; currentSIQ = baseSIQ;
         
    for (auto& s : activeStatuses) {
        if (s.type == StatusType::StatModifier) {
            if (s.targetStat == StatName::ATK) currentAttack += s.value;
            if (s.targetStat == StatName::DEF) currentDefense += s.value;
            if (s.targetStat == StatName::SPD) currentSpeed += s.value;
            if (s.targetStat == StatName::BIQ) currentBIQ += s.value;
            if (s.targetStat == StatName::SIQ) currentSIQ += s.value;
        }
    }
    
    // --- STANCE OVERRIDES ---
    if (naturalAbility == PassiveID::ScrewDat && isAltStance) {
        // Brian's Strike Stance: 
        // BIQ becomes the higher of his current SIQ (buffs applied) or his Base SIQ (ignoring debuffs).
        currentBIQ = std::max(currentSIQ, baseSIQ); 
        
        // SIQ drops to 0 for evasion/support purposes, but maxStamina is untouched.
        currentSIQ = 0; 
    }
    // ------------------------

    currentAttack = std::max(1, currentAttack); currentDefense = std::max(0, currentDefense);
    currentSpeed = std::max(0, currentSpeed); currentBIQ = std::max(0, currentBIQ); currentSIQ = std::max(0, currentSIQ);
}

void Entity::resetStats() {
    activeStatuses.clear(); 
    isAltStance = false; screwDatStacks = 0; screwDatDecayTimer = 0; hitNullificationStacks = 0; shieldHP = 0;
    usedActiveAbilityThisTurn = false; usedActives.clear();
    calculateActiveStats();
}

void Entity::toggleStance() {
    isAltStance = !isAltStance; 
    combatMenu.swap(altCombatMenu); 
    
    if (naturalAbility == PassiveID::ScrewDat) {
        if (isAltStance) {
            screwDatDecayTimer = 0; 
            std::cout << name << " shifted into STRIKE STANCE! (Ultimate decay paused!)\n";
        } else {
            if (screwDatStacks > 0) { 
                screwDatDecayTimer = 2; 
                std::cout << name << " shifted into SUPPORT STANCE! (Ultimate stacks will decay in 2 rounds!)\n"; 
            } else {
                std::cout << name << " shifted into SUPPORT STANCE!\n"; 
            }
        }
    } else {
        if (isAltStance) std::cout << name << " shifted into their Alternate Stance!\n";
        else std::cout << name << " shifted into their Normal Stance!\n";
    }
    // Always recalculate stats when a stance changes!
    calculateActiveStats(); 
}

void Entity::regenerateStamina() {
    int regenAmount = (currentSIQ / 4);          
    for (auto& s : activeStatuses) {
        if (s.type == StatusType::StaminaPenalty) regenAmount = regenAmount * (100 - s.value) / 100; 
    }
    currentStamina += regenAmount; if (currentStamina > maxStamina) currentStamina = maxStamina;
}

void Entity::checkBadges() {
    if (totalDamageTaken >= 100 && !hasScrapedKneesBadge) { hasScrapedKneesBadge = true; baseDefense += 5; std::cout << "\n[BADGE] " << name << " earned 'Scraped Knees'! (Base Defense +5)\n"; }
    if (totalDodges >= 5 && !hasStreetSmartBadge) { hasStreetSmartBadge = true; baseBIQ += 10; std::cout << "\n[BADGE] " << name << " earned 'Street Smart'! (Base BIQ +10)\n"; }
}

void Entity::gainEXP(int expAmount) {
    if (level >= 15) { currentEXP = 0; return; }
    currentEXP += expAmount; std::cout << "\n>>> [EXP] " << name << " gained " << expAmount << " EXP!\n";
    while (currentEXP >= expToNextLevel) {
        currentEXP -= expToNextLevel; levelUp();
        if (level >= 15) { std::cout << "\n" << name << " HAS REACHED MAX LEVEL!\n"; currentEXP = 0; break; }
    }
}

void Entity::levelUp() {
    level++; expToNextLevel = expToNextLevel * 1.5;
    
    int oldMaxHP = maxHP;
    int oldMaxStamina = maxStamina;

    if (internalID == EntityID::YoungBrian) { maxHP += 17; baseAttack += 4; baseDefense += 2; baseSpeed += 3; baseBIQ += 2; baseSIQ += 3; }
    else if (internalID == EntityID::YoungPaul) { maxHP += 12; baseAttack += 3; baseDefense += 2; baseSpeed += 3; baseBIQ += 3; baseSIQ += 2; }
    else if (internalID == EntityID::YoungVince) { maxHP += 20; baseAttack += 4; baseDefense += 3; baseSpeed += 1; baseBIQ += 2; baseSIQ += 1; }
    else if (internalID == EntityID::YoungJoe) { maxHP += 14; baseAttack += 4; baseDefense += 1; baseSpeed += 3; baseBIQ += 2; baseSIQ += 2; }
    else if (internalID == EntityID::YoungJustin) { maxHP += 19; baseAttack += 2; baseDefense += 3; baseSpeed += 3; baseBIQ += 2; baseSIQ += 1; }

    maxStamina = baseSIQ * 2;
    
    currentHP += (maxHP - oldMaxHP);
    currentStamina += (maxStamina - oldMaxStamina);

    std::cout << ">>> " << name << " grew to Level " << level << "! <<<\n";
    calculateActiveStats();
}