#include "CharacterRoster.h"
#include "MoveDatabase.h"
#include "AbilityDatabase.h"

static Entity buildBaseStats(EntityID internalID, ActorID actorID, std::string name, LifeStage stage, bool isBoss,
                      int hp, int atk, int def, int spd, int biq, int siq,
                      NaturalID naturalAbility) {
    Entity e(name, stage, isBoss);
    e.internalID = internalID; e.actorID = actorID; 
    e.maxHP = hp; e.currentHP = hp;
    e.baseAttack = atk; e.baseDefense = def; e.baseSpeed = spd;
    e.baseBIQ = biq; e.baseSIQ = siq; 
    
    // Stamina is now automatically generated based on the SIQ parameter
    e.maxStamina = siq * 2; 
    e.currentStamina = e.maxStamina;
    
    e.naturalAbility = naturalAbility; e.hiddenAbility = PassiveID::None;
    
    // ---> FIX: Instantly populates current Attack, Defense, etc.
    e.calculateActiveStats(); 
    return e;
}

// --- YOUNG CREW (LEVEL 1) ---
Entity buildYoungBrian() {
    Entity brian = buildBaseStats(EntityID::YoungBrian, ActorID::Brian, "Brian", LifeStage::Young, false, 55, 15, 11, 13, 9, 15, NaturalID::ScrewDat);
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
    Entity paul = buildBaseStats(EntityID::YoungPaul, ActorID::Paul, "Paul", LifeStage::Young, false, 39, 11, 8, 12, 15, 9, NaturalID::CrashOut);
    paul.level = 1;
    paul.combatMenu.push_back(getMove(MoveID::OneTwoPunch)); 
    paul.combatMenu.push_back(getMove(MoveID::Guard)); 
    return paul;
}

Entity buildYoungVince() {
    Entity vince = buildBaseStats(EntityID::YoungVince, ActorID::Vince, "Vince", LifeStage::Young, false, 65, 14, 11, 8, 10, 8, NaturalID::Mediator);
    vince.level = 1;
    vince.combatMenu.push_back(getMove(MoveID::Shove)); 
    vince.combatMenu.push_back(getMove(MoveID::Guard)); 
    return vince;
}

// Other characters
Entity buildYoungJoe() {
    Entity joe = buildBaseStats(EntityID::YoungJoe, ActorID::Joe, "Joe", LifeStage::Young, false, 46, 14, 8, 14, 11, 11, NaturalID::CrashOut);
    joe.level = 1;

    joe.combatMenu.push_back(getMove(MoveID::Windmill));
    joe.combatMenu.push_back(getMove(MoveID::Guard));
    return joe;
}

Entity buildYoungJustin() {
    Entity justin = buildBaseStats(EntityID::YoungJustin, ActorID::Justin, "Justin", LifeStage::Young, false, 59, 10, 11, 14, 11, 8, NaturalID::Mediator);
    justin.level = 1;

    justin.combatMenu.push_back(getMove(MoveID::Tackle));
    justin.combatMenu.push_back(getMove(MoveID::Guard));
    return justin;
}

// --- ADULT CREW (LEVEL 50) ---
Entity buildBrian() {
    Entity brian = buildBaseStats(EntityID::AdultBrian, ActorID::Brian, "Brian", LifeStage::Adult, false, 850, 190, 180, 180, 60, 100, NaturalID::ScrewDat);
    brian.level = 50; 
    brian.activeAbilities.push_back(ActiveID::AnalyzeWeakness);
    
    // Default to Support Stance natively 
    brian.combatMenu.push_back(getMove(MoveID::BrianSupportBasic)); 
    brian.combatMenu.push_back(getMove(MoveID::BrianSupportSkill)); 
    brian.combatMenu.push_back(getMove(MoveID::BrianSupportUlt));
    brian.combatMenu.push_back(getMove(MoveID::Guard)); 
    
    // Alt is Strike Stance
    brian.altCombatMenu.push_back(getMove(MoveID::BrianStrikeBasic)); 
    brian.altCombatMenu.push_back(getMove(MoveID::BrianStrikeSkill)); 
    brian.altCombatMenu.push_back(getMove(MoveID::BrianStrikeUlt));
    brian.altCombatMenu.push_back(getMove(MoveID::Guard));
    return brian;
}

Entity buildPaul() {
    Entity paul = buildBaseStats(EntityID::AdultPaul, ActorID::Paul, "Paul", LifeStage::Adult, false, 650, 150, 140, 140, 100, 60, NaturalID::CrashOut);
    paul.level = 50;
    paul.activeAbilities.push_back(ActiveID::Adrenaline); paul.activeAbilities.push_back(ActiveID::RecklessAbandon);

    paul.combatMenu.push_back(getMove(MoveID::PaulBasic)); 
    paul.combatMenu.push_back(getMove(MoveID::BobAndWeave)); 
    paul.combatMenu.push_back(getMove(MoveID::Haymaker));
    paul.combatMenu.push_back(getMove(MoveID::Guard));
    return paul;
}

Entity buildVince() {
    Entity vince = buildBaseStats(EntityID::AdultVince, ActorID::Vince, "Vince", LifeStage::Adult, false, 1000, 170, 190, 100, 65, 50, NaturalID::Mediator);
    vince.level = 50;
    vince.activeAbilities.push_back(ActiveID::Taunt);

    vince.combatMenu.push_back(getMove(MoveID::VinceBasic)); 
    vince.combatMenu.push_back(getMove(MoveID::Sledgehammer)); 
    vince.combatMenu.push_back(getMove(MoveID::HoldTheLine));
    vince.combatMenu.push_back(getMove(MoveID::Guard));
    return vince;
}