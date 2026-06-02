#include "TeamUps.h"
#include <algorithm>

std::vector<TeamUpSkill> getActiveTeamUps(std::vector<Entity*>& team, std::vector<TeamUpSkill>& masterList) {
    std::vector<TeamUpSkill> activeList;
    activeList.reserve(masterList.size()); // <--- NEW: Grabs exact memory needed
    
    std::vector<ActorID> aliveActors;
    aliveActors.reserve(team.size());      // <--- NEW: Grabs exact memory needed
    
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
        // Added EffectTiming::PreHit so it ignores defense BEFORE the massive hit
        {MoveID::AllOutAttack, "All-Out Attack", 0, 10, 7.0f, MoveTarget::OneEnemy, Effect::IgnoreDefense, EffectTiming::PreHit, MoveCategory::TeamUp, false} 
    });
    
    masterList.push_back({
        "Brothers in Arms", 
        {ActorID::Brian, ActorID::Paul}, 
        50,
        // Added EffectTiming::PostHit to this one
        {MoveID::BrothersInArms, "Brothers in Arms", 0, 6, 5.0f, MoveTarget::OneEnemy, Effect::None, EffectTiming::PostHit, MoveCategory::TeamUp, false}
    });
    
    return masterList;
}