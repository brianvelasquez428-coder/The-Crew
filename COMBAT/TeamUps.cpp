#include "TeamUps.h"
#include <algorithm>

std::vector<TeamUpSkill> getActiveTeamUps(std::vector<Entity*>& team, std::vector<TeamUpSkill>& masterList) {
    std::vector<TeamUpSkill> activeList;
    std::vector<ActorID> aliveActors;
    
    // Store alive ActorIDs instead of string names
    for (Entity* member : team) {
        if (member->isAlive) aliveActors.push_back(member->actorID);
    }
    
    for (TeamUpSkill& skill : masterList) {
        bool canUse = true;
        for (ActorID req : skill.requiredMembers) {
            if (std::find(aliveActors.begin(), aliveActors.end(), req) == aliveActors.end()) {
                canUse = false; 
                break;
            }
        }
        if (canUse) activeList.push_back(skill);
    }
    return activeList;
}

std::vector<TeamUpSkill> buildMasterTeamUps() {
    std::vector<TeamUpSkill> masterList;
    
    masterList.push_back({
        "The Crew's All-Out Attack", 
        {ActorID::Brian, ActorID::Paul, ActorID::Vince}, 
        100,
        {MoveID::AllOutAttack, "All-Out Attack", 0, 10, MoveCategory::TeamUp, MoveTarget::OneEnemy, Effect::IgnoreDefense, 7.0f} 
    });
    
    masterList.push_back({
        "Brothers in Arms", 
        {ActorID::Brian, ActorID::Paul}, 
        50,
        {MoveID::BrothersInArms, "Brothers in Arms", 0, 6, MoveCategory::TeamUp, MoveTarget::OneEnemy, Effect::DefenseScalingDamage, 5.0f} 
    });
    
    masterList.push_back({
        "Heavy Hitters", 
        {ActorID::Paul, ActorID::Vince}, 
        50,
        {MoveID::HeavyHitters, "Heavy Hitters", 0, 4, MoveCategory::TeamUp, MoveTarget::OneEnemy, Effect::LightsOutStun, 5.0f} 
    });
    
    return masterList;
}