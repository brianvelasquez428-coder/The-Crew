#pragma once
#include "Entity.h"
#include <string>
#include <vector>

std::string getPassiveName(PassiveID id);
std::string getActiveName(ActiveID id);
std::string getPassiveDescription(PassiveID id);
std::string getActiveDescription(ActiveID id);

extern const std::vector<PassiveID> MASTER_PASSIVE_POOL;
extern const std::vector<ActiveID> MASTER_ACTIVE_POOL;

void executeAbility(ActiveID effectID, Entity& caster, Entity& target);
void executeMoveEffect(Effect effect, Entity& caster, Entity& target);
void triggerPassives(Entity& activeCharacter);
void applyStartOfBattlePassives(Entity& character);
void triggerOnDodge(Entity& dodgingCharacter);