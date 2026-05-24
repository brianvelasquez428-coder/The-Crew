#pragma once
#include <string>
#include <vector>
#include "Entity.h"

struct TeamUpSkill {
    std::string name;
    std::vector<ActorID> requiredMembers; // Now strictly typed 
    int momentumCost; 
    Move moveData;                             
};

std::vector<TeamUpSkill> getActiveTeamUps(std::vector<Entity*>& team, std::vector<TeamUpSkill>& masterList);
std::vector<TeamUpSkill> buildMasterTeamUps();