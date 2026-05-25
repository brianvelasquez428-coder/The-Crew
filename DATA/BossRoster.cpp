#include "BossRoster.h"
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

Entity buildTony() {
    Entity boss = buildBaseStats(EntityID::BossTony, ActorID::Tony, "Tony", LifeStage::Adult, true, 1500, 240, 260, 220, 150, 110, PassiveID::Sparring);
    boss.activeAbilities.push_back(ActiveID::BodyWork);
    boss.activeAbilities.push_back(ActiveID::SlippingPunches);
    
    boss.combatMenu.push_back( getMove(MoveID::TonyP1Basic) );
    boss.combatMenu.push_back( getMove(MoveID::PrecisionStrike) );
    boss.combatMenu.push_back( getMove(MoveID::OlderBrother) );
    
    PhaseData tonyPhase2;
    tonyPhase2.thresholdHP = 750;
    tonyPhase2.transitionText = "Tony drops his guard, plants his feet, and cracks his knuckles. The sparring match is over.";
    tonyPhase2.newAttack = boss.baseAttack; tonyPhase2.newDefense = boss.baseDefense; tonyPhase2.newSpeed = boss.baseSpeed;
    tonyPhase2.newBIQ = boss.baseBIQ; tonyPhase2.newSIQ = boss.baseSIQ;
    tonyPhase2.newNaturalAbility = PassiveID::UnstoppableAssault;
    
    tonyPhase2.newActiveAbilities.push_back(ActiveID::RealityCheck);
    tonyPhase2.newActiveAbilities.push_back(ActiveID::TheDeepEnd);
    tonyPhase2.newActiveAbilities.push_back(ActiveID::BodyWork);
    tonyPhase2.newActiveAbilities.push_back(ActiveID::SlippingPunches);
    
    tonyPhase2.newCombatMenu.push_back( getMove(MoveID::TonyP2Basic) );
    tonyPhase2.newCombatMenu.push_back( getMove(MoveID::Clutch) );
    tonyPhase2.newCombatMenu.push_back( getMove(MoveID::LightsOut) );
    
    boss.extraPhases.push_back(tonyPhase2);
    return boss;
}