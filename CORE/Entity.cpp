#include "Entity.h"
#include "Logger.h"
#include <iostream>
#include <algorithm>

Entity::Entity(std::string spawnName, LifeStage spawnStage, bool spawnIsBoss) {
    name = spawnName; 
    internalID = EntityID::Unknown; 
    actorID = ActorID::Enemy;       
    currentStage = spawnStage; currentPhase = 1; isBoss = spawnIsBoss;
    
    
    // ---> FIX: Use nullptr instead of "None"
    grabbedBy = nullptr; isGrabbed = false; shieldHP = 0; 
    
    naturalAbility = NaturalID::None; naturalActiveUsedThisBattle = false; hasCrashedOut = false; mediatorAwakened = false; 
    usedActiveAbilityThisTurn = false; usedActives.clear(); isAltStance = false;           

    tempDamageBonus = 0; hitNullificationStacks = 0; screwDatStacks = 0; supportHealStacks = 0; screwDatDecayTimer = 0;
    currentHP = maxHP; isAlive = true;      
    currentAttack = baseAttack; currentDefense = baseDefense; currentSpeed = baseSpeed; currentStamina = maxStamina;
    level = 1; currentEXP = 0; expToNextLevel = 100; expDropValue = 0; moneyDropValue = 0; 
}

void Entity::addStatus(StatusEffect s) {
    if (!s.isStackable) {
        for (auto& existing : activeStatuses) {
            if (existing.id == s.id) {
                existing.duration = s.duration; 
                existing.statModifier = s.statModifier; // Updated payload name
                calculateActiveStats();
                return;
            }
        }
    }
    
    // UI Popup Logic is now entirely data-driven!
    bool isBuff = (s.category == StatusCategory::Buff);
    std::string pText = s.name; // Reads the name directly from the struct
    
    if (s.category == StatusCategory::Buff || s.category == StatusCategory::Debuff) {
        std::string statName = "";
        if (s.targetStat == StatName::ATK) statName = "ATK";
        else if (s.targetStat == StatName::DEF) statName = "DEF";
        else if (s.targetStat == StatName::SPD) statName = "SPD";
        else if (s.targetStat == StatName::BIQ) statName = "BIQ";
        else if (s.targetStat == StatName::SIQ) statName = "SIQ";
        
        if (s.statModifier > 0) pText = statName + " UP";
        else pText = statName + " DOWN";
    }
    
    pendingPopups.push_back({pText, isBuff});
    activeStatuses.push_back(s);
    calculateActiveStats();
}

bool Entity::hasStatus(StatusCategory category) {
    for (auto& s : activeStatuses) if (s.category == category) return true;
    return false;
}

bool Entity::hasStatusID(StatusID id) {
    for (auto& s : activeStatuses) if (s.id == id) return true;
    return false;
}

void Entity::removeStatusByType(StatusCategory category) {
    activeStatuses.erase(std::remove_if(activeStatuses.begin(), activeStatuses.end(),
        [category](const StatusEffect& s) { return s.category == category; }), activeStatuses.end());
    calculateActiveStats();
}

int Entity::getDamageReduction() {
    int dr = 0;
    
    // Mediator Logic
    if (naturalAbility == NaturalID::Mediator) {
        dr += mediatorAwakened ? 30 : 15;
    }
    
    // Future-proofing: You can easily add a loop here later that checks activeStatuses 
    // for generic "Damage Reduction" buffs without ever touching takeDamage again!
    
    return dr;
}

void Entity::triggerOnHitPassives() {
    if (!isAlive) return;

    // TONY'S SPARRING
    if (naturalAbility == NaturalID::Sparring) {
        int defBonus = std::max(1, (int)(baseDefense * 0.05));
        addStatus({StatusID::SparringDEF, "Sparring DEF", StatusCategory::Buff, StatName::DEF, defBonus, 0, false, false, false, 1, true});
        std::cout << "   [SPARRING] " << name << " adjusts their guard! (DEF +5%)\n";
        pendingLogMessages.push_back("[SPARRING] " + name + " adjusts their guard! (DEF +5%)");
    }

    // PAUL'S CRASH OUT
    if (naturalAbility == NaturalID::CrashOut) {
        int atkBonus = std::max(1, (int)(baseAttack * 0.05));
        int spdBonus = std::max(1, (int)(baseSpeed * 0.05));
        addStatus({StatusID::CrashOutHitBuffATK, "Crash Out ATK", StatusCategory::Buff, StatName::ATK, atkBonus, 0, false, false, false, 99, true});
        addStatus({StatusID::CrashOutHitBuffSPD, "Crash Out SPD", StatusCategory::Buff, StatName::SPD, spdBonus, 0, false, false, false, 99, true});

        int currentStacks = 0;
        for (auto& s : activeStatuses) {
            if (s.id == StatusID::CrashOutHitBuffATK) currentStacks++;
        }
        pendingLogMessages.push_back("[CRASH OUT] " + name + " takes a hit!");
        pendingLogMessages.push_back("            (ATK & SPD +5%, Stack " + std::to_string(currentStacks) + ")");

        // Crash Out Burst Check
        if (!hasCrashedOut && ((float)currentHP / maxHP) < 0.60f) {
            hasCrashedOut = true;
            addStatus({StatusID::CrashOutATK, "Crash Out Burst", StatusCategory::Buff, StatName::ATK, (int)(baseAttack * 0.50), 0, false, false, false, 3, false});
            addStatus({StatusID::CrashOutDEF, "Crash Out Guard", StatusCategory::Buff, StatName::DEF, (int)(baseDefense * 0.50), 0, false, false, false, 3, false});
            std::cout << "\n>>> PAUL IS CRASHING OUT! (ATK & DEF +50% for 3 turns!) <<<\n";
            pendingLogMessages.push_back("[WARNING] " + name + " IS CRASHING OUT!");
            pendingLogMessages.push_back("          (ATK & DEF +50% for 3 turns!)");
        }
    }
}

int Entity::takeDamage(int rawDamage, bool isCritical) {
    if (shieldHP > 0 && isCriticalOnlyShield == true) {
        if (!isCritical) { 
            std::cout << name << "'s filter absorbed the attack! (0 Damage)\n"; 
            return 0; 
        }
        else std::cout << "Critical Strike Pierces though the filter!\n";
    }
         
    float defenseMultiplier = 100.0f / (100.0f + currentDefense);       
    float calculatedDamage = rawDamage * defenseMultiplier;                    
    
    if (isCritical) calculatedDamage *= 2.0f;
    
    float variance = 1.0f + (((rand() % 21) - 10) / 100.0f);
    calculatedDamage *= variance;
    
    int finalDamage = std::max(1, (int)calculatedDamage);
    
    // --- NEW: CLEAN DAMAGE REDUCTION HOOK ---
    int drPercent = getDamageReduction();
    if (drPercent > 0) {
        finalDamage = (int)(finalDamage * (1.0f - (drPercent / 100.0f)));
    }
    // ----------------------------------------
    
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
    
    if (currentHP <= 0) { 
        currentHP = 0; isAlive = false; 
        std::cout << "\n>>> " << name << " has been knocked out! <<<\n"; 
    }

    // --- NEW: CLEAN ON-HIT PASSIVE HOOK ---
    triggerOnHitPassives();
    // --------------------------------------

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
        
        // Change this line to match the new enum
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
    std::vector<StatusEffect> pendingStatuses; // <-- 1. Create a temporary holding bin

    for (auto it = activeStatuses.begin(); it != activeStatuses.end(); ) {
        if (it->dotDamage > 0) {
            GameLog::Add("[" + it->name + "] " + name + " loses " + std::to_string(it->dotDamage) + " HP!");
            currentHP -= it->dotDamage;
            if (currentHP <= 0) { currentHP = 0; isAlive = false; GameLog::Add(">>> " + name + " succumbed to their wounds! <<<"); }
        }
        
        it->duration--;
        if (it->duration <= 0) {
            if (it->causesStun) GameLog::Add(name + " shook off the stun and is ready to fight!");
            else if (it->causesTaunt) GameLog::Add(name + " is no longer drawing attacks.");
            else if (it->halvesStaminaRegen) GameLog::Add(name + " catches their breath! (Stamina Regen restored)");
            else GameLog::Add(name + "'s [" + it->name + "] wore off.");
            
            // Paul's Exhaustion check
            if (it->id == StatusID::CrashOutDEF) {
                GameLog::Add(name + "'s adrenaline fades... the crash out is over.");
                // <-- 2. Push to the temporary bin instead of calling addStatus directly!
                pendingStatuses.push_back({StatusID::CrashOutExhaustion, "Crash Out Exhaustion", StatusCategory::Debuff, StatName::DEF, (int)(-baseDefense * 0.50), 0, false, false, false, 2, false});
            }
            
            it = activeStatuses.erase(it);
        } else {
            ++it;
        }
    }

    // <-- 3. Safely add the pending statuses now that the loop is over!
    for (const auto& status : pendingStatuses) {
        addStatus(status);
    }
    
    if (naturalAbility == NaturalID::ScrewDat) {
        if (isAltStance) {
            // Decay timer resets to 2 ONLY if you actually end your turn in Strike
            screwDatDecayTimer = 2;
        } else {
            // THE FIX: If you have stacks, force the countdown even if it hits negatives
            if (screwDatStacks > 0) {
                screwDatDecayTimer--;
                
                if (screwDatDecayTimer <= 0) { 
                    screwDatStacks = 0; 
                    screwDatDecayTimer = 0; // Clamp it back to 0 just to be clean
                    std::cout << "   [FADED] " << name << "'s Strike Ultimate momentum has completely vanished!\n"; 
                    GameLog::Add("[SCREW DAT] Strike Ultimate momentum has completely vanished!"); 
                }
            }
        }
    }
    calculateActiveStats();
    regenerateStamina();
}

void Entity::calculateActiveStats() {
    currentAttack = baseAttack; currentDefense = baseDefense; currentSpeed = baseSpeed;
    currentBIQ = baseBIQ; currentSIQ = baseSIQ;
         
    // --- THE FIX: MOVE STANCE OVERRIDES BEFORE THE BUFF LOOP ---
    if (naturalAbility == NaturalID::ScrewDat) {
        if (isAltStance) {
            // Sets his starting BIQ to his Base SIQ BEFORE buffs apply
            currentBIQ = baseSIQ; 
            // Drops SIQ to 0
            currentSIQ = 0; 
        }
    }
    
    for (auto& s : activeStatuses) {
        // Read Categories instead of specific Types, and use the new statModifier variable
        if (s.category == StatusCategory::Buff || s.category == StatusCategory::Debuff) {
            if (s.targetStat == StatName::ATK) currentAttack += s.statModifier;
            if (s.targetStat == StatName::DEF) currentDefense += s.statModifier;
            if (s.targetStat == StatName::SPD) currentSpeed += s.statModifier;
            if (s.targetStat == StatName::BIQ) currentBIQ += s.statModifier;
            if (s.targetStat == StatName::SIQ) currentSIQ += s.statModifier;
        }
    }

    currentAttack = std::max(1, currentAttack); currentDefense = std::max(0, currentDefense);
    currentSpeed = std::max(0, currentSpeed); currentBIQ = std::max(0, currentBIQ); currentSIQ = std::max(0, currentSIQ);
}

void Entity::resetStats() {
    activeStatuses.clear();
    
    // isAltStance = false; <--- DELETE THIS ENTIRE LINE!
    usedActiveAbilityThisTurn = false; 
    usedActives.clear();
    
    // --- THE FIX: WIPE ALL NATURAL ABILITY MEMORY ---
    naturalActiveUsedThisBattle = false; 
    hasCrashedOut = false; 
    mediatorAwakened = false;
    
    screwDatStacks = 0; 
    screwDatDecayTimer = 0; 
    supportHealStacks = 0; 
    
    hitNullificationStacks = 0; 
    shieldHP = 0;
    
    currentAttack = baseAttack; 
    currentDefense = baseDefense; 
    currentSpeed = baseSpeed;
    currentBIQ = baseBIQ; 
    currentSIQ = baseSIQ;
}

void Entity::toggleStance() {
    isAltStance = !isAltStance; 
    combatMenu.swap(altCombatMenu); 
    
    // THE FIX: We completely removed the screwDatDecayTimer logic from here.
    // Changing stances no longer instantly manipulates the decay!
    
    if (naturalAbility == NaturalID::ScrewDat) {
        if (isAltStance) std::cout << name << " shifted into STRIKE STANCE!\n";
        else std::cout << name << " shifted into SUPPORT STANCE!\n"; 
    } else {
        if (isAltStance) std::cout << name << " shifted into their Alternate Stance!\n";
        else std::cout << name << " shifted into their Normal Stance!\n";
    }
    
    calculateActiveStats(); 
}

void Entity::regenerateStamina() {
    int regenAmount = (currentSIQ / 4);          
    for (auto& s : activeStatuses) {
        // Checking the direct payload flag instead of a generic StatusType
        if (s.halvesStaminaRegen) regenAmount = regenAmount * (100 - s.statModifier) / 100; 
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