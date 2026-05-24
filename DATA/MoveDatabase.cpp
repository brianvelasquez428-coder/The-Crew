#include "MoveDatabase.h"
#include <iostream>

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

static Move buildBasic(MoveID id, std::string name, int stamina, int hits, MoveTarget target = MoveTarget::OneEnemy, Effect effect = Effect::None, float power = 1.0f) {
    return {id, name, stamina, hits, MoveCategory::Basic, target, effect, power};
}

static Move buildSkill(MoveID id, std::string name, int stamina, int hits, MoveTarget target = MoveTarget::OneEnemy, Effect effect = Effect::None, float power = 1.5f) {
    return {id, name, stamina, hits, MoveCategory::Skill, target, effect, power};
}

static Move buildUlt(MoveID id, std::string name, int stamina, int hits, MoveTarget target = MoveTarget::OneEnemy, Effect effect = Effect::None, float power = 2.5f) {
    return {id, name, stamina, hits, MoveCategory::Ultimate, target, effect, power};
}

static Move buildSupport(MoveID id, std::string name, int stamina, MoveTarget target, Effect effect) {
    return {id, name, stamina, 0, MoveCategory::Support, target, effect, 1.0f};
}

Move getMove(MoveID moveID) {
    switch (moveID) {
        // --- UNIVERSAL / ENEMIES ---
        case MoveID::Strike: return buildBasic(moveID, "Strike", 10, 1);
        case MoveID::TakeCover: return buildSupport(moveID, "Take Cover", 0, MoveTarget::Self, Effect::RestoreStamina);
        case MoveID::SupportStrike: return buildBasic(moveID, "Support Strike", 0, 1, MoveTarget::OneEnemy, Effect::None, 0.8f);
        case MoveID::ClumsySwing: return buildBasic(moveID, "Clumsy Swing", 10, 1);

        // --- PAUL ---
        case MoveID::PaulBasic: return buildBasic(moveID, "Basic Attack", 0, 4);
        case MoveID::OneTwoPunch: return buildBasic(moveID, "1-2 Punch", 15, 2, MoveTarget::OneEnemy, Effect::None, 1.2f);
        case MoveID::BobAndWeave: return buildSkill(moveID, "Sweeping Hook", 40, 2, MoveTarget::TwoEnemies);
        case MoveID::KnifeSlash: return buildSkill(moveID, "Knife Slash", 25, 1, MoveTarget::OneEnemy, Effect::ApplyBleed, 1.2f);
        case MoveID::Haymaker: return buildUlt(moveID, "Haymaker", 85, 8, MoveTarget::OneEnemy, Effect::IgnoreDefense, 2.5f);

        // --- BRIAN ---
        case MoveID::BrianSupportBasic: return buildBasic(moveID, "Basic (Support)", 0, 3, MoveTarget::OneEnemy, Effect::RestoreStamina);
        case MoveID::BrianSupportSkill: return buildSupport(moveID, "Support Skill (+40% DEF & Nullify)", 40, MoveTarget::OneAlly, Effect::DefenseBuff40);
        case MoveID::BrianSupportUlt: return buildUlt(moveID, "Support Ultimate", 120, 0, MoveTarget::AllAllies, Effect::HealAndCleanse, 1.0f);
        case MoveID::BrianStrikeBasic: return buildBasic(moveID, "Basic (Strike)", 0, 4, MoveTarget::OneEnemy, Effect::LowerPriority, 1.2f);
        case MoveID::BrianStrikeSkill: return buildSkill(moveID, "Strike Skill", 40, 1, MoveTarget::OneEnemy, Effect::StrikeDefenseDebuff, 2.5f);
        case MoveID::BrianStrikeUlt: return buildUlt(moveID, "Strike Ultimate", 120, 1, MoveTarget::OneEnemy, Effect::StrikeUlt, 3.5f);

        // --- VINCE ---
        case MoveID::VinceBasic: return buildBasic(moveID, "Basic Attack", 0, 2, MoveTarget::OneEnemy, Effect::SlowEnemy);
        case MoveID::Shove: return buildBasic(moveID, "Shove", 10, 1);
        case MoveID::MiniSledge: return buildBasic(moveID, "Mini Sledge", 30, 1, MoveTarget::OneEnemy, Effect::DefenseScalingDamage, 1.5f);
        case MoveID::Sledgehammer: return buildSkill(moveID, "Sledgehammer", 35, 1, MoveTarget::OneEnemy, Effect::DefenseScalingDamage, 1.0f);
        case MoveID::HoldTheLine: return buildSupport(moveID, "Hold The Line", 80, MoveTarget::AllAllies, Effect::HoldLineShield);

        // --- TONY ---
        case MoveID::TonyP1Basic: return buildBasic(moveID, "Basic Attack", 0, 2, MoveTarget::OneEnemy, Effect::None, 2.5f);
        case MoveID::PrecisionStrike: return buildSkill(moveID, "Precision Strike", 40, 1, MoveTarget::OneEnemy, Effect::PrecisionStrikeDebuff, 4.0f);
        case MoveID::OlderBrother: return buildUlt(moveID, "Older Brother", 90, 1, MoveTarget::OneEnemy, Effect::StaminaStrip, 6.0f);
        case MoveID::TonyP2Basic: return buildBasic(moveID, "Basic Attack", 0, 3, MoveTarget::OneEnemy, Effect::P2BasicDebuff, 2.5f);
        case MoveID::Clutch: return buildSkill(moveID, "Clutch", 60, 1, MoveTarget::OneEnemy, Effect::ClutchGrab, 4.0f);
        case MoveID::LightsOut: return buildUlt(moveID, "Lights Out", 100, 1, MoveTarget::OneEnemy, Effect::LightsOutStun, 7.0f);

        case MoveID::None: return buildBasic(moveID, "None", 0, 0);
        default:
            std::cout << "ERROR: MoveID not found in Database!\n";
            return buildBasic(MoveID::Unknown, "Unknown Move", 0, 1);
    }
}

std::string getMoveName(MoveID moveID) {
    return getMove(moveID).name;
}