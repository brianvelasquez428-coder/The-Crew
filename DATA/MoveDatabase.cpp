#include "MoveDatabase.h"
#include <iostream>
#include <map> // Add this to the top of MoveDatabase.cpp

std::string getCategoryName(MoveCategory category) {
    switch(category) {
        case MoveCategory::Basic: return "Basic";
        case MoveCategory::Skill: return "Skill";
        case MoveCategory::Ultimate: return "Ultimate";
        case MoveCategory::Support: return "Support";
        case MoveCategory::TeamUp: return "TeamUp";
        default: return "Unknown";
    }
}

// ---> PASTE THE CENTRALIZED TEXT FUNCTIONS HERE <---
std::string getTargetText(MoveTarget t) { 
    switch(t) {      
        case MoveTarget::Self: return "Self";      
        case MoveTarget::OneEnemy: return "1 Enemy";      
        case MoveTarget::TwoEnemies: return "2 Enemies";      
        case MoveTarget::AllEnemies: return "All Enemies";      
        case MoveTarget::OneAlly: return "1 Ally";      
        case MoveTarget::TwoAllies: return "2 Allies";      
        case MoveTarget::AllAllies: return "All Allies";      
        default: return "Unknown"; 
    } 
} 

std::string getEffectText(Effect e) { 
    switch(e) {      
        case Effect::None: return "No special effect.";      
        case Effect::RestoreStamina: return "Restores 25 Stamina.";      
        case Effect::DefenseScalingDamage: return "User's Defense is added to raw damage.";      
        case Effect::SelfAttackBuff10: return "Increases user's Attack by 10%.";      
        case Effect::ApplyBleed: return "Applies Bleed (5 DMG/turn) for 3 turns.";      
        case Effect::IgnoreDefense: return "Ignores 50% of the target's Defense.";      
        case Effect::DefenseBuff40: return "Increases Defense by 40% and adds 1 Hit Nullification.";      
        case Effect::HealAndCleanse: return "Heals 40% HP and cures all status conditions.";      
        case Effect::LowerPriority: return "Reduces target's Speed by 15 for 2 turns.";      
        case Effect::StrikeDefenseDebuff: return "Reduces target's Defense by 40% for 2 turns.";      
        case Effect::StrikeUlt: return "Guaranteed Hit. Consumes stacks for massive damage.";      
        case Effect::SlowEnemy: return "Reduces target's Speed by 20 for 2 turns.";      
        case Effect::HoldLineShield: return "Applies a 60 HP Shield.";      
        case Effect::PrecisionStrikeDebuff: return "Reduces target's DEF by 15 and BIQ by 10 for 3 turns.";      
        case Effect::StaminaStrip: return "Halves target's Stamina Regeneration for 2 turns.";      
        case Effect::P2BasicDebuff: return "Phase 2 Enhanced Basic Attack.";      
        case Effect::ClutchGrab: return "Grabs target, preventing escape.";      
        case Effect::LightsOutStun: return "Stuns the target for 1 turn.";      
        default: return "Unknown effect."; 
    } 
}
// ---------------------------------------------------

// Offensive moves default to PostHit
static Move buildBasic(MoveID id, std::string name, int stamina, int hits, MoveTarget target = MoveTarget::OneEnemy, Effect effect = Effect::None, float power = 1.0f, bool ignoresEvasion = false, EffectTiming timing = EffectTiming::PostHit) {
    return {id, name, stamina, hits, power, target, effect, timing, MoveCategory::Basic, ignoresEvasion};
}

static Move buildSkill(MoveID id, std::string name, int stamina, int hits, MoveTarget target = MoveTarget::OneEnemy, Effect effect = Effect::None, float power = 1.5f, bool ignoresEvasion = false, EffectTiming timing = EffectTiming::PostHit) {
    return {id, name, stamina, hits, power, target, effect, timing, MoveCategory::Skill, ignoresEvasion};
}

static Move buildUlt(MoveID id, std::string name, int stamina, int hits, MoveTarget target = MoveTarget::OneEnemy, Effect effect = Effect::None, float power = 2.5f, bool ignoresEvasion = false, EffectTiming timing = EffectTiming::PostHit) {
    return {id, name, stamina, hits, power, target, effect, timing, MoveCategory::Ultimate, ignoresEvasion};
}

// Support moves default to OnCast (since they don't do damage)
static Move buildSupport(MoveID id, std::string name, int stamina, MoveTarget target, Effect effect, bool ignoresEvasion = true, EffectTiming timing = EffectTiming::OnCast) {
    return {id, name, stamina, 0, 0.0f, target, effect, timing, MoveCategory::Support, ignoresEvasion};
}

const Move& getMove(MoveID moveID) {
    // 'static' ensures this map is only built ONCE when the game boots up.
    static const std::map<MoveID, Move> database = {
        // --- UNIVERSAL / ENEMIES ---
        { MoveID::Strike, buildBasic(MoveID::Strike, "Strike", 5, 1, MoveTarget::OneEnemy, Effect::None, 0.5f) },
        { MoveID::Guard, buildSupport(MoveID::Guard, "Guard", 0, MoveTarget::Self, Effect::RestoreStamina) },
        { MoveID::ClumsySwing, buildBasic(MoveID::ClumsySwing, "Clumsy Swing", 10, 1) },
        { MoveID::None, buildBasic(MoveID::None, "None", 0, 0) },

        // --- PAUL ---
        { MoveID::OneTwoPunch, buildBasic(MoveID::OneTwoPunch, "1-2 Punch", 10, 2, MoveTarget::OneEnemy, Effect::None, 0.6f) },
        { MoveID::PaulBasic, buildBasic(MoveID::PaulBasic, "Basic Attack", 0, 4, MoveTarget::OneEnemy, Effect::SelfAttackBuff10, 0.5f) },
        { MoveID::BobAndWeave, buildSkill(MoveID::BobAndWeave, "Sweeping Hook", 40, 1, MoveTarget::TwoEnemies, Effect::None, 1.4f) },
        { MoveID::KnifeSlash, buildSkill(MoveID::KnifeSlash, "Knife Slash", 25, 1, MoveTarget::OneEnemy, Effect::ApplyBleed, 1.5f) },
        { MoveID::Haymaker, buildUlt(MoveID::Haymaker, "Haymaker", 85, 8, MoveTarget::OneEnemy, Effect::IgnoreDefense, 0.8f) },

        // --- BRIAN ---
        { MoveID::StepOff, buildBasic(MoveID::StepOff, "Step Off", 5, 1, MoveTarget::OneEnemy, Effect::None, 0.6f) },
        { MoveID::FaintPunches, buildBasic(MoveID::FaintPunches, "Faint Punches", 15, 2, MoveTarget::OneEnemy, Effect::None, 0.5f) },
        { MoveID::BrianSupportBasic, buildBasic(MoveID::BrianSupportBasic, "Basic (Support)", 0, 3, MoveTarget::OneEnemy, Effect::RestoreStamina, 0.7f) },
        { MoveID::BrianSupportSkill, buildSupport(MoveID::BrianSupportSkill, "Support Skill (+40% DEF & Nullify)", 40, MoveTarget::OneAlly, Effect::DefenseBuff40) },
        { MoveID::BrianSupportUlt, buildUlt(MoveID::BrianSupportUlt, "Support Ultimate", 120, 0, MoveTarget::AllAllies, Effect::HealAndCleanse, 1.0f, true, EffectTiming::OnCast) },
        { MoveID::BrianStrikeBasic, buildBasic(MoveID::BrianStrikeBasic, "Basic (Strike)", 0, 4, MoveTarget::OneEnemy, Effect::LowerPriority, 0.5f) },
        { MoveID::BrianStrikeSkill, buildSkill(MoveID::BrianStrikeSkill, "Armor Piercer", 25, 1, MoveTarget::OneEnemy, Effect::StrikeDefenseDebuff, 1.5f, false, EffectTiming::PreHit) },
        { MoveID::BrianStrikeUlt, buildUlt(MoveID::BrianStrikeUlt, "Strike Ultimate", 120, 1, MoveTarget::OneEnemy, Effect::StrikeUlt, 3.0f, true) },

        // --- VINCE ---
        { MoveID::Shove, buildBasic(MoveID::Shove, "Shove", 10, 1, MoveTarget::OneEnemy, Effect::None, 0.8f) },
        { MoveID::MiniSledge, buildBasic(MoveID::MiniSledge, "Mini Sledge", 30, 1, MoveTarget::OneEnemy, Effect::DefenseScalingDamage, 1.5f) },
        { MoveID::VinceBasic, buildBasic(MoveID::VinceBasic, "Basic Attack", 0, 2, MoveTarget::OneEnemy, Effect::SlowEnemy, 0.8f) },
        { MoveID::Sledgehammer, buildSkill(MoveID::Sledgehammer, "Sledgehammer", 35, 1, MoveTarget::OneEnemy, Effect::DefenseScalingDamage, 1.5f) },
        { MoveID::HoldTheLine, buildSupport(MoveID::HoldTheLine, "Hold The Line", 80, MoveTarget::AllAllies, Effect::HoldLineShield) },
        { MoveID::Windmill, buildBasic(MoveID::Windmill, "Windmill", 10, 3, MoveTarget::OneEnemy, Effect::None, 0.4f) },
        { MoveID::Tackle, buildBasic(MoveID::Tackle, "Tackle", 7, 1, MoveTarget::OneEnemy, Effect::None, 0.9f) },

        // --- TONY ---
        { MoveID::TonyP1Basic, buildBasic(MoveID::TonyP1Basic, "Basic Attack", 0, 2, MoveTarget::OneEnemy, Effect::None, 1.0f) },
        { MoveID::PrecisionStrike, buildSkill(MoveID::PrecisionStrike, "Precision Strike", 40, 1, MoveTarget::OneEnemy, Effect::PrecisionStrikeDebuff, 2.0f) },
        { MoveID::OlderBrother, buildUlt(MoveID::OlderBrother, "Older Brother", 80, 1, MoveTarget::OneEnemy, Effect::StaminaStrip, 3.5f) },
        { MoveID::TonyP2Basic, buildBasic(MoveID::TonyP2Basic, "Basic Attack", 0, 3, MoveTarget::OneEnemy, Effect::P2BasicDebuff, 1.0f) },
        { MoveID::Clutch, buildSkill(MoveID::Clutch, "Clutch", 40, 1, MoveTarget::OneEnemy, Effect::ClutchGrab, 2.0f) },
        { MoveID::LightsOut, buildUlt(MoveID::LightsOut, "Lights Out", 100, 1, MoveTarget::OneEnemy, Effect::LightsOutStun, 4.0f) }
    };

    // Find the move in the map
    auto it = database.find(moveID);
    if (it != database.end()) {
        return it->second;
    }

    // Fallback if the move isn't found
    std::cout << "ERROR: MoveID not found in Database!\n";
    static const Move fallback = buildBasic(MoveID::Unknown, "Unknown Move", 0, 1);
    return fallback;
}

std::string getMoveName(MoveID moveID) {
    return getMove(moveID).name;
}