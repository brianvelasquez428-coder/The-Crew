#include "Entity.h"
#include "AbilityDatabase.h"
#include "../Systems/Logger.h"
#include <iostream>
#include <algorithm>

const std::vector<NaturalID> MASTER_NATURAL_POOL = {
    NaturalID::None, NaturalID::ScrewDat, NaturalID::CrashOut, 
    NaturalID::Mediator, NaturalID::Sparring, NaturalID::UnstoppableAssault
};

const std::vector<PassiveID> MASTER_PASSIVE_POOL = { 
    PassiveID::None, PassiveID::LightOnTheFeet, PassiveID::HeavyHands, PassiveID::IronWill, PassiveID::SlippingPunches 
};

const std::vector<ActiveID> MASTER_ACTIVE_POOL = { 
    ActiveID::None, ActiveID::Adrenaline, ActiveID::Taunt, ActiveID::BodyWork, ActiveID::SlippingPunches, 
    ActiveID::TheDeepEnd, ActiveID::AnalyzeWeakness, ActiveID::RecklessAbandon, ActiveID::RealityCheck 
};

std::string getNaturalName(NaturalID id) {
    switch(id) {
        case NaturalID::ScrewDat: return "Screw Dat";
        case NaturalID::CrashOut: return "Crash Out";
        case NaturalID::Mediator: return "Mediator";
        case NaturalID::Sparring: return "Sparring";
        case NaturalID::UnstoppableAssault: return "Unstoppable Assault";
        default: return "None";
    }
}

std::string getNaturalDescription(NaturalID id) {
    switch(id) {
        case NaturalID::ScrewDat: return "(SUPPORT): Evade uses SIQ. Team gains stacking Max HP/Heal (+1% or 1 HP) every turn.\n(STRIKE): BIQ equals Base SIQ. ATK/BIQ +5% every turn. Ult power +30% per turn (Max 120%).";
        case NaturalID::CrashOut: return "ATK/DEF +50% for 3 turns when HP < 60%. Afterwards, DEF -50% for 2 turns.\nEvery hit received grants a permanent +5% ATK/SPD.";
        case NaturalID::Mediator: return "PASSIVE: 15% Damage Reduction. Additional +15% DR when an ally's HP < 20%.\nACTIVE: (If ally <20% HP or alone) Heals 50%, immune to stun, Taunts for 1 turn.";
        case NaturalID::Sparring: return "ATTACK +5% for every consecutive attack in a combo (resets after move).\nDEFENSE +5% for every attack received (resets next turn).";
        case NaturalID::UnstoppableAssault: return "Tony drops his guard. Base Attack and Base Speed are permanently increased by +10%.";
        default: return "No description available.";
    }
}

std::string getPassiveName(PassiveID id) {
    switch(id) {
        case PassiveID::LightOnTheFeet: return "Light on the Feet";
        case PassiveID::HeavyHands: return "Heavy Hands";
        case PassiveID::IronWill: return "Iron Will";
        case PassiveID::SlippingPunches: return "Slipping Punches";
        case PassiveID::Combo: return "Combo";
        default: return "None";
    }
}

std::string getPassiveDescription(PassiveID id) {
    switch(id) {
        case PassiveID::LightOnTheFeet: return "Speed +40 at the start of battle.";
        case PassiveID::HeavyHands: return "Attack +25 at the start of battle.";
        case PassiveID::IronWill: return "Defense +50 at the start of battle.";
        case PassiveID::SlippingPunches: return "Recover 8 Stamina every time you dodge an attack.";
        case PassiveID::Combo: return "Chance to unleash extra strikes during an attack.";
        default: return "No description available.";
    }
}

std::string getActiveName(ActiveID id) {
    switch(id) {
        case ActiveID::Adrenaline: return "Adrenaline";
        case ActiveID::Taunt: return "Taunt";
        case ActiveID::BodyWork: return "Body Work";
        case ActiveID::SlippingPunches: return "Slipping Punches";
        case ActiveID::TheDeepEnd: return "The Deep End";
        case ActiveID::AnalyzeWeakness: return "Analyze Weakness";
        case ActiveID::RecklessAbandon: return "Reckless Abandon";
        case ActiveID::RealityCheck: return "Reality Check";
        default: return "None";
    }
}

std::string getActiveDescription(ActiveID id) {
    switch(id) {
        case ActiveID::Adrenaline: return "Instantly recover 40 Stamina.";
        case ActiveID::Taunt: return "Draw all enemy attacks. Target gains +20 and +40 DEF.";
        case ActiveID::BodyWork: return "Target loses 20 Stamina immediately.";
        case ActiveID::TheDeepEnd: return "Target loses -30 Strategic and Battle IQ for 3 turns.";
        case ActiveID::AnalyzeWeakness: return "Scans the enemy to reveal hidden traits.";
        case ActiveID::RecklessAbandon: return "Sacrifice 20% HP for a massive 3-turn Attack buff.";
        case ActiveID::RealityCheck: return "Removes all buffs from the target.";
        case ActiveID::SlippingPunches: return "Gain a massive dodge chance increase for 1 turn.";
        default: return "No description available.";
    }
}

void executeAbility(ActiveID effectID, Entity& caster, Entity& target) {
    if (effectID == ActiveID::Adrenaline) { 
        GameLog::Add("[ACTIVE] " + caster.name + " gets a burst of energy! (+40 Stamina)");
        caster.currentStamina += 40; if (caster.currentStamina > caster.maxStamina) caster.currentStamina = caster.maxStamina;
        
        // ---> THE FIX: Push STM UP Popup <---
        caster.pendingPopups.push_back({"STM UP", true});
    }
    else if (effectID == ActiveID::Taunt) { 
        GameLog::Add("[ACTIVE] " + caster.name + " draws aggro! (Taunt, ATK +20, DEF +40 for 2 turns)");
        // --- FIX: Changed the last argument to 'true' so the stats stack! ---
        caster.addStatus({StatusID::TauntedATK, "Taunting ATK", StatusCategory::Buff, StatName::ATK, 20, 0, false, false, false, 2, true});
        caster.addStatus({StatusID::TauntedDEF, "Taunting DEF", StatusCategory::Buff, StatName::DEF, 40, 0, false, false, false, 2, true});
        caster.addStatus({StatusID::Taunting, "TAUNTING", StatusCategory::Special, StatName::None, 0, 0, false, true, false, 2, false});
    }
    else if (effectID == ActiveID::BodyWork) {
        GameLog::Add("[ACTIVE] " + caster.name + " targets the ribs! (" + target.name + " loses 20 Stamina)");
        target.currentStamina -= 20; if (target.currentStamina < 0) target.currentStamina = 0;
        
        // ---> THE FIX: Push STM DOWN Popup <---
        target.pendingPopups.push_back({"STM DOWN", false});
    }
    else if (effectID == ActiveID::SlippingPunches) {
        GameLog::Add("[ACTIVE] " + caster.name + " focuses on evasion! (Dodge activates Stamina Regen)");
    }
    else if (effectID == ActiveID::TheDeepEnd) {
        GameLog::Add("[ACTIVE] " + caster.name + " stares down " + target.name + "! (SIQ and BIQ -30)");
        // --- FIX: Changed to stackable ---
        target.addStatus({StatusID::DeepEndSIQ, "Deep End SIQ", StatusCategory::Debuff, StatName::SIQ, -30, 0, false, false, false, 3, true});
        target.addStatus({StatusID::DeepEndBIQ, "Deep End BIQ", StatusCategory::Debuff, StatName::BIQ, -30, 0, false, false, false, 3, true});
    }
}

void executeMoveEffect(Effect effect, Entity& caster, Entity& target) {
    switch (effect) {
        case Effect::RestoreStamina: 
             caster.currentStamina += 25; if (caster.currentStamina > caster.maxStamina) caster.currentStamina = caster.maxStamina; 
             GameLog::Add("[EFFECT] " + caster.name + " recovers 25 Stamina!"); 
             caster.pendingPopups.push_back({"STM UP", true});
             break;
             
        // ---> THE FIX: Just leave the text! The math is handled by the engine now. <---
        case Effect::DefenseScalingDamage: 
             GameLog::Add("[EFFECT] " + caster.name + " uses their Defense to power the attack!"); 
             break;
        case Effect::ApplyBleed: 
            target.addStatus({StatusID::Bleed, "Bleed", StatusCategory::DoT, StatName::None, 0, 5, false, false, false, 3, true}); 
            GameLog::Add("[EFFECT] " + target.name + " is bleeding! (Takes 5 DMG per round for 3 turns)"); break;
        case Effect::SelfAttackBuff10: 
            caster.addStatus({StatusID::BasicAttackBuff, "1-2 Punch ATK", StatusCategory::Buff, StatName::ATK, (int)(caster.baseAttack * 0.10), 0, false, false, false, 99, true}); 
            GameLog::Add("[EFFECT] " + caster.name + " builds momentum! (ATK +10%)"); 
            break;
        case Effect::DefenseBuff40: 
             target.addStatus({StatusID::Hardened, "Hardened", StatusCategory::Buff, StatName::DEF, (int)(target.baseDefense * 0.40), 0, false, false, false, 3, true}); 
             target.hitNullificationStacks = 1; 
             GameLog::Add("[EFFECT] " + target.name + "'s Defense rises by 40% and they gain 1 Hit Nullification!"); 
             
             // ---> THE FIX: Push Null Gained Popup <---
             target.pendingPopups.push_back({"NULL GAINED", true});
             break;
        case Effect::HealAndCleanse: 
             target.healHP(target.maxHP * 0.40); 
             for (auto it = target.activeStatuses.begin(); it != target.activeStatuses.end(); ) {
                 bool isDebuff = false;
                 if (it->category == StatusCategory::Debuff || it->category == StatusCategory::DoT || it->causesStun || it->halvesStaminaRegen) isDebuff = true;
                 if (isDebuff) it = target.activeStatuses.erase(it);
                 else ++it;
             }
             target.calculateActiveStats(); 
             GameLog::Add("[EFFECT] " + target.name + " is healed (40%) and cured of all negative conditions!"); 
             break;
        case Effect::LowerPriority: 
            target.addStatus({StatusID::Slowed, "Slowed", StatusCategory::Debuff, StatName::SPD, -15, 0, false, false, false, 2, true}); 
            GameLog::Add("[EFFECT] " + target.name + "'s priority is lowered! (Speed -15 for 2 turns)"); break;
        case Effect::StrikeDefenseDebuff: 
            target.addStatus({StatusID::GuardBroken, "Guard Broken", StatusCategory::Debuff, StatName::DEF, (int)(-target.baseDefense * 0.40), 0, false, false, false, 2, true}); 
            GameLog::Add("[EFFECT] " + target.name + "'s guard is broken! (DEF -40% for 2 turns)"); break;
        case Effect::SlowEnemy: 
            target.addStatus({StatusID::BoggedDown, "Bogged Down", StatusCategory::Debuff, StatName::SPD, -20, 0, false, false, false, 2, true}); 
            GameLog::Add("[EFFECT] " + target.name + " is bogged down! (Speed -20 for 2 turns)"); break;
        case Effect::HoldLineShield:  
             target.shieldHP = caster.maxHP * 0.25; 
             GameLog::Add("[EFFECT] " + caster.name + " shields " + target.name + " for " + std::to_string(target.shieldHP) + " HP!"); break;
        case Effect::PrecisionStrikeDebuff: 
            target.addStatus({StatusID::PrecisionDEFDrop, "Precision DEF Drop", StatusCategory::Debuff, StatName::DEF, -15, 0, false, false, false, 3, true}); 
            target.addStatus({StatusID::PrecisionBIQDrop, "Precision BIQ Drop", StatusCategory::Debuff, StatName::BIQ, -10, 0, false, false, false, 3, true}); 
            GameLog::Add("[EFFECT] Tony strikes a nerve! (" + target.name + "'s DEF -15 and BIQ -10 for 3 turns)"); break;
        case Effect::StaminaStrip: 
            target.addStatus({StatusID::WindKnockedOut, "Wind Knocked Out", StatusCategory::Debuff, StatName::None, 0, 0, false, false, true, 2, true}); 
            GameLog::Add("[EFFECT] " + caster.name + " knocks the wind out of " + target.name + "! (Stamina Regen halved for 2 turns)");            
             // ---> THE FIX: Push the REGEN DOWN popup! <---
             target.pendingPopups.push_back({"REGEN DOWN", false});
             break;
        case Effect::ClutchGrab: 
             target.isGrabbed = true; target.grabbedBy = &caster; 
             GameLog::Add("[EFFECT] " + caster.name + " grabs " + target.name + "! They can't escape!"); break;
        case Effect::LightsOutStun:  
             if (!target.hasStatusID(StatusID::StunImmunity)) { 
                 target.addStatus({StatusID::Stunned, "STUNNED", StatusCategory::HardCC, StatName::None, 0, 0, true, false, false, 1, false}); 
                 GameLog::Add("[EFFECT] " + target.name + " is STUNNED!"); 
             } else { GameLog::Add("[EFFECT] " + target.name + " resisted the stun!"); }
             break;
        case Effect::None: case Effect::StrikeUlt: case Effect::P2BasicDebuff: default: break;
    }
}

void applyStartOfBattlePassives(Entity& character) {
    if (character.naturalAbility == NaturalID::ScrewDat && character.isAltStance) {
        character.screwDatDecayTimer = 2;
    }
    
    for (PassiveID passive : character.passiveAbilities) {
        if (passive == PassiveID::LightOnTheFeet) { 
            character.addStatus({StatusID::LightFeet, "Light Feet", StatusCategory::Buff, StatName::SPD, 40, 0, false, false, false, 99, false}); 
            GameLog::Add("[PASSIVE] " + character.name + " stays light on their feet! (Speed +40)"); 
        } 
        else if (passive == PassiveID::HeavyHands) { 
            character.addStatus({StatusID::HeavyHands, "Heavy Hands", StatusCategory::Buff, StatName::ATK, 25, 0, false, false, false, 99, false}); 
            GameLog::Add("[PASSIVE] " + character.name + " wraps their hands tight! (Attack +25)"); 
        } 
        else if (passive == PassiveID::IronWill) { 
            character.addStatus({StatusID::IronWill, "Iron Will", StatusCategory::Buff, StatName::DEF, 50, 0, false, false, false, 99, false}); 
            GameLog::Add("[PASSIVE] " + character.name + " hardens their stance! (Defense +50)"); 
        }
    }
}

void triggerOnDodge(Entity& dodgingCharacter) {
    bool hasSlippingPunches = false;
    if (std::find(dodgingCharacter.passiveAbilities.begin(), dodgingCharacter.passiveAbilities.end(), PassiveID::SlippingPunches) != dodgingCharacter.passiveAbilities.end()) hasSlippingPunches = true;
    if (std::find(dodgingCharacter.activeAbilities.begin(), dodgingCharacter.activeAbilities.end(), ActiveID::SlippingPunches) != dodgingCharacter.activeAbilities.end()) hasSlippingPunches = true;
    if (hasSlippingPunches) {
        dodgingCharacter.currentStamina += 8; if (dodgingCharacter.currentStamina > dodgingCharacter.maxStamina) dodgingCharacter.currentStamina = dodgingCharacter.maxStamina;
        std::cout << "   [PASSIVE] Slipping Punches: " << dodgingCharacter.name << " recovers 8 Stamina!\n";
    }
}

void applyNaturalPassives(Entity& character, std::vector<Entity*>& team) {
    if (character.naturalAbility == NaturalID::ScrewDat) {
        if (character.isAltStance) { // STRIKE STANCE
            // Max 4 stacks (120%)
            if (character.screwDatStacks < 4) character.screwDatStacks++; 
            
            int atkBonus = std::max(1, (int)(character.baseAttack * 0.05)); 
            int biqBonus = std::max(1, (int)(character.baseBIQ * 0.05));
            
            character.addStatus({StatusID::DemolitionistATK, "Demolitionist ATK", StatusCategory::Buff, StatName::ATK, atkBonus, 0, false, false, false, 99, true});
            character.addStatus({StatusID::DemolitionistBIQ, "Demolitionist BIQ", StatusCategory::Buff, StatName::BIQ, biqBonus, 0, false, false, false, 99, true});
            
            GameLog::Add("[SCREW DAT] " + character.name + " builds momentum! (ATK & BIQ +5%)");
            GameLog::Add("   -> Strike Ultimate Decay: PAUSED"); // <--- NEW UI TRACKER
            
        } else { // SUPPORT STANCE
            // Increment the dedicated support stack (this one NEVER decays)
            character.supportHealStacks++; 
            
            for(Entity* ally : team) {
                if (ally->isAlive) {
                    float healMultiplier = character.supportHealStacks * 0.01f;
                    int healAmount = std::max(1, (int)(ally->maxHP * healMultiplier));
                    ally->healHP(healAmount); 
                }
            }
            GameLog::Add("[SCREW DAT] The crew passively recovers health! (Heal Power: " + std::to_string(character.supportHealStacks) + "%)");
            
            // --- NEW UI TRACKER ---
            if (character.screwDatStacks > 0) {
                if (character.screwDatDecayTimer == 1) {
                    GameLog::Add("   -> Strike Ultimate Decay: 1 round left (Fades at end of this turn!)");
                } else {
                    GameLog::Add("   -> Strike Ultimate Decay: " + std::to_string(character.screwDatDecayTimer) + " round(s) remaining");
                }
            }
        }
    }
    else if (character.naturalAbility == NaturalID::Mediator) {
        if (!character.mediatorAwakened) {
            for (Entity* ally : team) {
                // Awakening checks if ally is dead OR below 20%
                if (ally != &character && (!ally->isAlive || ((float)ally->currentHP / ally->maxHP) <= 0.20f)) {
                    character.mediatorAwakened = true;
                    GameLog::Add("[MEDIATOR] " + character.name + " refuses to let his friends fall! (Damage Reduction +30%)");
                    break;
                }
            }
        }
    }
}

bool canUseNaturalActive(Entity& character, std::vector<Entity*>& team) {
    if (character.naturalAbility == NaturalID::Mediator) {
        // --- THE FIX: PERMANENT UNLOCK ---
        // If he already woke up this battle, the button stays unlocked forever!
        if (character.mediatorAwakened) return true;

        // Otherwise, do the standard check
        for (Entity* ally : team) {
            if (ally != &character && (!ally->isAlive || ((float)ally->currentHP / ally->maxHP) <= 0.20f)) {
                // If it triggers here, lock the boolean in so he stays awake!
                character.mediatorAwakened = true; 
                return true;
            }
        }
        return false;
    }
    return false;
}

void executeNaturalActive(Entity& character, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam) {
    if (character.naturalAbility == NaturalID::Mediator) {
        GameLog::Add(">>> " + character.name + " steps in to take the heat! <<<");
        
        character.healHP(character.maxHP * 0.50); // Heals 50% HP
        
        GameLog::Add("[NATURAL] " + character.name + " draws aggro and braces for impact! (Taunt, ATK +20, DEF +40)");
        character.addStatus({StatusID::TauntedATK, "Taunting ATK", StatusCategory::Buff, StatName::ATK, 20, 0, false, false, false, 2, true});
        character.addStatus({StatusID::TauntedDEF, "Taunting DEF", StatusCategory::Buff, StatName::DEF, 40, 0, false, false, false, 2, true});
        character.addStatus({StatusID::Taunting, "TAUNTING", StatusCategory::Special, StatName::None, 0, 0, false, true, false, 2, false});
    }
}