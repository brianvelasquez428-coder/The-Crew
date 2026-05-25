#include "EnemyRoster.h"
#include "MoveDatabase.h"

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

Entity buildScrawnyThug() {
    Entity thug = buildBaseStats(EntityID::ScrawnyThug, ActorID::Enemy, "Scrawny Thug", LifeStage::Young, false,
                                   20, 6, 3, 5, 2, 2, PassiveID::None);
    thug.combatMenu.push_back( getMove(MoveID::ClumsySwing) );
    thug.expDropValue = 100; thug.moneyDropValue = 1; 
    return thug;
}

Entity buildStreetThug() {
    Entity thug = buildBaseStats(EntityID::StreetThug, ActorID::Enemy, "Street Thug", LifeStage::Young, false,
                                   20, 6, 3, 5, 2, 2, PassiveID::None);
    thug.combatMenu.push_back( getMove(MoveID::ClumsySwing) );
    thug.expDropValue = 100; thug.moneyDropValue = 1; 
    return thug;
}