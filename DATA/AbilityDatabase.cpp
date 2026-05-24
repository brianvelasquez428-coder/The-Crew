#include "AbilityDatabase.h"
#include <iostream>
#include <algorithm>

const std::vector<PassiveID> MASTER_PASSIVE_POOL = { 
    PassiveID::None, PassiveID::LightOnTheFeet, PassiveID::HeavyHands, PassiveID::IronWill, PassiveID::SlippingPunches 
};

const std::vector<ActiveID> MASTER_ACTIVE_POOL = { 
    ActiveID::None, ActiveID::Adrenaline, ActiveID::Taunt, ActiveID::BodyWork, ActiveID::SlippingPunches, 
    ActiveID::TheDeepEnd, ActiveID::AnalyzeWeakness, ActiveID::RecklessAbandon, ActiveID::RealityCheck 
};

std::string getPassiveName(PassiveID id) {
    switch(id) {
        case PassiveID::ScrewDat: return "Screw Dat";
        case PassiveID::CrashOut: return "Crash Out";
        case PassiveID::Mediator: return "Mediator";
        case PassiveID::Sparring: return "Sparring";
        case PassiveID::UnstoppableAssault: return "Unstoppable Assault";
        case PassiveID::LightOnTheFeet: return "Light on the Feet";
        case PassiveID::HeavyHands: return "Heavy Hands";
        case PassiveID::IronWill: return "Iron Will";
        case PassiveID::SlippingPunches: return "Slipping Punches";
        case PassiveID::Combo: return "Combo";
        default: return "None";
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

std::string getPassiveDescription(PassiveID id) {
    switch(id) {
        case PassiveID::ScrewDat: return "Restores 5% HP natively. Strike Stance: +5% ATK/BIQ instead.";
        case PassiveID::CrashOut: return "When HP drops below 60%, Attack and Defense skyrocket by 50%.";
        case PassiveID::UnstoppableAssault: return "Attack and Speed permanently increased by 10%.";
        case PassiveID::LightOnTheFeet: return "Speed +40 at the start of battle.";
        case PassiveID::HeavyHands: return "Attack +25 at the start of battle.";
        case PassiveID::IronWill: return "Defense +50 at the start of battle.";
        case PassiveID::SlippingPunches: return "Recover 8 Stamina every time you dodge an attack.";
        case PassiveID::Mediator: return "Immune to Stuns, +10% Defense when allies are low HP.";
        case PassiveID::Sparring: return "Takes reduced damage and studies the opponent's moves.";
        default: return "No description available.";
    }
}

std::string getActiveDescription(ActiveID id) {
    switch(id) {
        case ActiveID::Adrenaline: return "Instantly recover 40 Stamina.";
        case ActiveID::Taunt: return "Draw all enemy attacks. Target gains +20 ATK but loses -40 DEF.";
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
        std::cout << "   [ACTIVE] " << caster.name << " gets a burst of energy! (+40 Stamina)\n";
        caster.currentStamina += 40; if (caster.currentStamina > caster.maxStamina) caster.currentStamina = caster.maxStamina;
    }
    else if (effectID == ActiveID::Taunt) { 
        std::cout << "   [ACTIVE] " << caster.name << " taunts " << target.name << "! (Target ATK +20, Target DEF -40)\n";
        target.addStatus({StatusID::TauntedATK, StatusType::StatModifier, StatName::ATK, 20, 2, false});
        target.addStatus({StatusID::TauntedDEF, StatusType::StatModifier, StatName::DEF, -40, 2, false});
        caster.addStatus({StatusID::Taunting, StatusType::Taunt, StatName::None, 0, 2, false});
    }
    else if (effectID == ActiveID::BodyWork) {
        std::cout << "   [ACTIVE] " << caster.name << " targets the ribs! (" << target.name << " loses 20 Stamina)\n";
        target.currentStamina -= 20; if (target.currentStamina < 0) target.currentStamina = 0;
    }
    else if (effectID == ActiveID::SlippingPunches) {
        std::cout << "   [ACTIVE] " << caster.name << " focuses on evasion! (Dodge activates Stamina Regen)\n";
    }
    else if (effectID == ActiveID::TheDeepEnd) {
        std::cout << "   [ACTIVE] " << caster.name << " stares down " << target.name << ". (SIQ and BIQ -30!)\n";
        target.addStatus({StatusID::DeepEndSIQ, StatusType::StatModifier, StatName::SIQ, -30, 3, false});
        target.addStatus({StatusID::DeepEndBIQ, StatusType::StatModifier, StatName::BIQ, -30, 3, false});
    }
}

void executeMoveEffect(Effect effect, Entity& caster, Entity& target) {
    switch (effect) {
        case Effect::RestoreStamina: caster.currentStamina += 25; if (caster.currentStamina > caster.maxStamina) caster.currentStamina = caster.maxStamina; std::cout << "   [EFFECT] " << caster.name << " recovers 25 Stamina!\n"; break;
        case Effect::DefenseScalingDamage: caster.tempDamageBonus = caster.currentDefense; std::cout << "   [EFFECT] " << caster.name << " uses their Defense to power the attack!\n"; break;
        case Effect::ApplyBleed: target.addStatus({StatusID::Bleed, StatusType::Bleed, StatName::None, 5, 3, true}); std::cout << "   [EFFECT] " << target.name << " is bleeding! (Takes 5 damage per round)\n"; break;
        case Effect::IgnoreDefense: target.addStatus({StatusID::GuardBreak, StatusType::StatModifier, StatName::DEF, (int)(-target.currentDefense * 0.50), 1, false}); std::cout << "   [EFFECT] " << caster.name << " shatters the guard! (" << target.name << "'s DEF halved for this hit!)\n"; break;
        case Effect::DefenseBuff40: target.addStatus({StatusID::Hardened, StatusType::StatModifier, StatName::DEF, (int)(target.baseDefense * 0.40), 3, false}); target.hitNullificationStacks = 1; std::cout << "   [EFFECT] " << target.name << "'s Defense rises by 40% and they gain 1 Hit Nullification!\n"; break;
        case Effect::HealAndCleanse: target.healHP(target.maxHP * 0.40); target.activeStatuses.clear(); target.calculateActiveStats(); std::cout << "   [EFFECT] " << target.name << " is healed and cured of all status conditions!\n"; break;
        case Effect::LowerPriority: target.addStatus({StatusID::Slowed, StatusType::StatModifier, StatName::SPD, -15, 2, false}); std::cout << "   [EFFECT] " << target.name << "'s priority is lowered! (Speed -15)\n"; break;
        case Effect::StrikeDefenseDebuff: target.addStatus({StatusID::GuardBroken, StatusType::StatModifier, StatName::DEF, (int)(-target.baseDefense * 0.40), 2, false}); std::cout << "   [EFFECT] " << target.name << "'s guard is broken! (DEF -40%)\n"; break;
        case Effect::SlowEnemy: target.addStatus({StatusID::BoggedDown, StatusType::StatModifier, StatName::SPD, -20, 2, false}); std::cout << "   [EFFECT] " << target.name << " is bogged down! (Speed -20)\n"; break;
        
        // ---> FIX: Shield scales to 25% of the Caster's Max HP
        case Effect::HoldLineShield: 
             target.shieldHP = caster.maxHP * 0.25; 
             std::cout << "   [EFFECT] " << caster.name << " shields " << target.name << " for " << target.shieldHP << " HP!\n"; 
             break;
             
        case Effect::PrecisionStrikeDebuff: target.addStatus({StatusID::PrecisionDEFDrop, StatusType::StatModifier, StatName::DEF, -15, 3, false}); target.addStatus({StatusID::PrecisionBIQDrop, StatusType::StatModifier, StatName::BIQ, -10, 3, false}); std::cout << "   [EFFECT] Tony strikes a nerve! (" << target.name << "'s DEF and BIQ drop!)\n"; break;
        case Effect::StaminaStrip: target.addStatus({StatusID::WindKnockedOut, StatusType::StaminaPenalty, StatName::None, 50, 2, false}); std::cout << "   [EFFECT] " << caster.name << " knocks the wind out of " << target.name << "! (Stamina Regen halved!)\n"; break;
        
        // ---> FIX: Pass the memory address of the caster (&caster)
        case Effect::ClutchGrab: target.isGrabbed = true; target.grabbedBy = &caster; std::cout << "   [EFFECT] " << caster.name << " grabs " << target.name << "! They can't escape!\n"; break;
        
        case Effect::LightsOutStun: 
             if (!target.hasStatus(StatusType::StunImmune)) {
                 target.addStatus({StatusID::Stunned, StatusType::Stun, StatName::None, 0, 1, false}); std::cout << "   [EFFECT] " << target.name << " is stunned!\n"; 
             } else { std::cout << "   [EFFECT] " << target.name << " resisted the stun!\n"; }
            break;
        case Effect::None: case Effect::StrikeUlt: case Effect::P2BasicDebuff: default: break;
    }
}

void triggerPassives(Entity& activeCharacter) {
    if (activeCharacter.naturalAbility == PassiveID::ScrewDat) {
        if (activeCharacter.isAltStance) {
            activeCharacter.screwDatStacks += 2; 
            int atkBonus = activeCharacter.baseAttack * 0.05; int biqBonus = activeCharacter.baseBIQ * 0.05;
            activeCharacter.addStatus({StatusID::DemolitionistATK, StatusType::StatModifier, StatName::ATK, atkBonus, 99, true});
            activeCharacter.addStatus({StatusID::DemolitionistBIQ, StatusType::StatModifier, StatName::BIQ, biqBonus, 99, true});
            std::cout << "[PASSIVE] Screw Dat: " << activeCharacter.name << "'s Attack & BIQ permanently rise by 5%!\n";
        } else {
            int healAmount = activeCharacter.maxHP * 0.05;
            std::cout << "[PASSIVE] Screw Dat: " << activeCharacter.name << " naturally recovers " << healAmount << " HP!\n";
            activeCharacter.healHP(healAmount);
        }
    }
    else if (activeCharacter.naturalAbility == PassiveID::CrashOut) {
        if (activeCharacter.currentHP <= (activeCharacter.maxHP * 0.60)) {
            if (!activeCharacter.hasStatusID(StatusID::CrashOutATK)) {
                activeCharacter.addStatus({StatusID::CrashOutATK, StatusType::StatModifier, StatName::ATK, (int)(activeCharacter.baseAttack * 0.50), 2, false});
                activeCharacter.addStatus({StatusID::CrashOutDEF, StatusType::StatModifier, StatName::DEF, (int)(activeCharacter.baseDefense * 0.50), 2, false});
                std::cout << "[PASSIVE] CRASH OUT: " << activeCharacter.name << "'s HP is critical! Attack and Defense skyrocket by 50%!\n";
            }
        }
    }
    else if (activeCharacter.naturalAbility == PassiveID::UnstoppableAssault) {
        int atkBonus = activeCharacter.baseAttack * 0.10; int spdBonus = activeCharacter.baseSpeed * 0.10;
        activeCharacter.addStatus({StatusID::UnstoppableATK, StatusType::StatModifier, StatName::ATK, atkBonus, 99, true});
        activeCharacter.addStatus({StatusID::UnstoppableSPD, StatusType::StatModifier, StatName::SPD, spdBonus, 99, true});
        std::cout << "[PASSIVE] Unstoppable Assault: " << activeCharacter.name << "'s Attack and Speed increase by 10%!\n";
    }
}

void applyStartOfBattlePassives(Entity& character) {
    for (PassiveID passive : character.passiveAbilities) {
        if (passive == PassiveID::LightOnTheFeet) { character.addStatus({StatusID::LightFeet, StatusType::StatModifier, StatName::SPD, 40, 99, false}); std::cout << "[PASSIVE] " << character.name << " stays light on their feet! (Speed +40)\n"; } 
        else if (passive == PassiveID::HeavyHands) { character.addStatus({StatusID::HeavyHands, StatusType::StatModifier, StatName::ATK, 25, 99, false}); std::cout << "[PASSIVE] " << character.name << " wraps their hands tight! (Attack +25)\n"; } 
        else if (passive == PassiveID::IronWill) { character.addStatus({StatusID::IronWill, StatusType::StatModifier, StatName::DEF, 50, 99, false}); std::cout << "[PASSIVE] " << character.name << " hardens their stance! (Defense +50)\n"; }
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