#include "EnemyRoster.h"
#include "MoveDatabase.h"

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

Entity buildScrawnyThug() {
    Entity thug = buildBaseStats(EntityID::ScrawnyThug, ActorID::ScrawnyThug, "Scrawny Thug", LifeStage::Young, false,
                                   20, 6, 3, 5, 2, 3, NaturalID::None);

    thug.combatMenu.push_back( getMove(MoveID::Strike));
    thug.combatMenu.push_back( getMove(MoveID::Guard));

    thug.expDropValue = 100; thug.moneyDropValue = 1; 
    return thug;
}

Entity buildStreetThug() {
    Entity thug = buildBaseStats(EntityID::StreetThug, ActorID::StreetThug, "Street Thug", LifeStage::Young, false,
                                   20, 6, 3, 5, 2, 3, NaturalID::None);

    thug.combatMenu.push_back( getMove(MoveID::Strike));
    thug.combatMenu.push_back( getMove(MoveID::Guard));

    thug.expDropValue = 100; thug.moneyDropValue = 1; 
    return thug;
}