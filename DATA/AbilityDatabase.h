#pragma once
#include "Entity.h"
#include <string>
#include <vector>

std::string getNaturalName(NaturalID id);
std::string getPassiveName(PassiveID id);
std::string getActiveName(ActiveID id);
std::string getNaturalDescription(NaturalID id);
std::string getPassiveDescription(PassiveID id);
std::string getActiveDescription(ActiveID id);

extern const std::vector<PassiveID> MASTER_PASSIVE_POOL;
extern const std::vector<ActiveID> MASTER_ACTIVE_POOL;
extern const std::vector<NaturalID> MASTER_NATURAL_POOL;

void executeAbility(ActiveID effectID, Entity& caster, Entity& target);
void executeMoveEffect(Effect effect, Entity& caster, Entity& target);
void applyStartOfBattlePassives(Entity& character);
void triggerOnDodge(Entity& dodgingCharacter);

void applyNaturalPassives(Entity& character, std::vector<Entity*>& team);
bool canUseNaturalActive(Entity& caster, std::vector<Entity*>& team);
void executeNaturalActive(Entity& caster, std::vector<Entity*>& team, std::vector<Entity*>& enemies);
