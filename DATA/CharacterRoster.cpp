#include "CharacterRoster.h"
#include "MoveDatabase.h"
#include "AbilityDatabase.h"

static Entity buildBaseStats(EntityID internalID, ActorID actorID, std::string name, LifeStage stage, bool isBoss,
                      int hp, int atk, int def, int spd, int biq, int siq,
                      PassiveID naturalAbility) {
    Entity e(name, stage, isBoss);
    e.internalID = internalID; e.actorID = actorID; 
    e.maxHP = hp; e.currentHP = hp;
    e.baseAttack = atk; e.baseDefense = def; e.baseSpeed = spd;
    e.baseBIQ = biq; e.baseSIQ = siq; 
    
    // Stamina is now automatically generated based on the SIQ parameter
    e.maxStamina = siq * 2; 
    e.currentStamina = e.maxStamina;
    
    e.naturalAbility = naturalAbility; e.hiddenAbility = PassiveID::None;
    return e;
}

// --- YOUNG CREW (LEVEL 1) ---
Entity buildYoungBrian() {
    Entity brian = buildBaseStats(EntityID::YoungBrian, ActorID::Brian, "Brian", LifeStage::Young, false, 55, 15, 11, 13, 9, 15, PassiveID::ScrewDat);
    brian.level = 1; 
    
    // Default to Support Stance natively 
    brian.combatMenu.push_back(getMove(MoveID::StepOff)); 
    brian.combatMenu.push_back(getMove(MoveID::Guard)); 
    
    // Alt is Strike Stance
    brian.altCombatMenu.push_back(getMove(MoveID::FaintPunches)); 
    brian.altCombatMenu.push_back(getMove(MoveID::Guard)); 
    return brian;
}

Entity buildYoungPaul() {
    Entity paul = buildBaseStats(EntityID::YoungPaul, ActorID::Paul, "Paul", LifeStage::Young, false, 39, 11, 8, 12, 15, 9, PassiveID::CrashOut);
    paul.level = 1;
    paul.combatMenu.push_back(getMove(MoveID::OneTwoPunch)); 
    paul.combatMenu.push_back(getMove(MoveID::Guard)); 
    return paul;
}

Entity buildYoungVince() {
    Entity vince = buildBaseStats(EntityID::YoungVince, ActorID::Vince, "Vince", LifeStage::Young, false, 65, 14, 11, 8, 10, 8, PassiveID::Mediator);
    vince.level = 1;
    vince.combatMenu.push_back(getMove(MoveID::Shove)); 
    vince.combatMenu.push_back(getMove(MoveID::Guard)); 
    return vince;
}

// --- ADULT CREW (LEVEL 50) ---
Entity buildBrian() {
    Entity brian = buildBaseStats(EntityID::AdultBrian, ActorID::Brian, "Brian", LifeStage::Adult, false, 800, 150, 160, 140, 120, 130, PassiveID::ScrewDat);
    brian.level = 50; 
    brian.activeAbilities.push_back(ActiveID::AnalyzeWeakness);
    
    // Default to Support Stance natively 
    brian.combatMenu.push_back(getMove(MoveID::BrianSupportBasic)); 
    brian.combatMenu.push_back(getMove(MoveID::BrianSupportSkill)); 
    brian.combatMenu.push_back(getMove(MoveID::BrianSupportUlt));
    
    // Alt is Strike Stance
    brian.altCombatMenu.push_back(getMove(MoveID::BrianStrikeBasic)); 
    brian.altCombatMenu.push_back(getMove(MoveID::BrianStrikeSkill)); 
    brian.altCombatMenu.push_back(getMove(MoveID::BrianStrikeUlt));
    return brian;
}

Entity buildPaul() {
    Entity paul = buildBaseStats(EntityID::AdultPaul, ActorID::Paul, "Paul", LifeStage::Adult, false, 750, 190, 140, 200, 100, 80, PassiveID::CrashOut);
    paul.level = 50;
    paul.activeAbilities.push_back(ActiveID::Adrenaline); paul.activeAbilities.push_back(ActiveID::RecklessAbandon);
    paul.combatMenu.push_back(getMove(MoveID::PaulBasic)); 
    paul.combatMenu.push_back(getMove(MoveID::BobAndWeave)); 
    paul.combatMenu.push_back(getMove(MoveID::KnifeSlash)); 
    paul.combatMenu.push_back(getMove(MoveID::Haymaker));
    return paul;
}

Entity buildVince() {
    Entity vince = buildBaseStats(EntityID::AdultVince, ActorID::Vince, "Vince", LifeStage::Adult, false, 999, 170, 190, 100, 65, 50, PassiveID::Mediator);
    vince.level = 50;
    vince.activeAbilities.push_back(ActiveID::Taunt);
    vince.combatMenu.push_back(getMove(MoveID::VinceBasic)); 
    vince.combatMenu.push_back(getMove(MoveID::Sledgehammer)); 
    vince.combatMenu.push_back(getMove(MoveID::HoldTheLine));
    return vince;
}