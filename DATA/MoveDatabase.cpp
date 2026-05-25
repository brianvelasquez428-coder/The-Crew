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
        case MoveID::Strike: return buildBasic(moveID, "Strike", 5, 1, MoveTarget::OneEnemy, Effect::None, 0.5f);
        case MoveID::Guard: return buildSupport(moveID, "Guard", 0, MoveTarget::Self, Effect::RestoreStamina);
        case MoveID::ClumsySwing: return buildBasic(moveID, "Clumsy Swing", 10, 1);
        case MoveID::None: return buildBasic(moveID, "None", 0, 0);

        // --- PAUL ---
        // Young
        case MoveID::OneTwoPunch: return buildBasic(moveID, "1-2 Punch", 10, 2, MoveTarget::OneEnemy, Effect::None, 0.6f);

        case MoveID::PaulBasic: return buildBasic(moveID, "Basic Attack", 0, 4, MoveTarget::OneEnemy, Effect::None, 0.5f); // does about 80 damage total to boss
        case MoveID::BobAndWeave: return buildSkill(moveID, "Sweeping Hook", 40, 1, MoveTarget::TwoEnemies, Effect::None, 1.4f); // does about 60 damage total to 1 boss
        case MoveID::KnifeSlash: return buildSkill(moveID, "Knife Slash", 25, 1, MoveTarget::OneEnemy, Effect::ApplyBleed, 1.5f);
        case MoveID::Haymaker: return buildUlt(moveID, "Haymaker", 85, 8, MoveTarget::OneEnemy, Effect::IgnoreDefense, 0.8f);

        // --- BRIAN ---
        // Young Support
        case MoveID::StepOff: return buildBasic(moveID, "Step Off", 5, 1, MoveTarget::OneEnemy, Effect::None, 0.6f);
        // Young Strike
        case MoveID::FaintPunches: return buildBasic(moveID, "Faint Punches", 15, 2, MoveTarget::OneEnemy, Effect::None, 0.5f);

        case MoveID::BrianSupportBasic: return buildBasic(moveID, "Basic (Support)", 0, 3, MoveTarget::OneEnemy, Effect::RestoreStamina, 0.7f); // does about 100 damage total to boss
        case MoveID::BrianSupportSkill: return buildSupport(moveID, "Support Skill (+40% DEF & Nullify)", 40, MoveTarget::OneAlly, Effect::DefenseBuff40);
        case MoveID::BrianSupportUlt: return buildUlt(moveID, "Support Ultimate", 120, 0, MoveTarget::AllAllies, Effect::HealAndCleanse, 1.0f);

        case MoveID::BrianStrikeBasic: return buildBasic(moveID, "Basic (Strike)", 0, 4, MoveTarget::OneEnemy, Effect::LowerPriority, 0.5f); // does about 100 damage total to boss
        case MoveID::BrianStrikeSkill: return buildSkill(moveID, "Strike Skill", 40, 1, MoveTarget::OneEnemy, Effect::StrikeDefenseDebuff, 1.5f);
        case MoveID::BrianStrikeUlt: return buildUlt(moveID, "Strike Ultimate", 120, 1, MoveTarget::OneEnemy, Effect::StrikeUlt, 3.0f);

        // --- VINCE ---
        // Young
        case MoveID::Shove: return buildBasic(moveID, "Shove", 10, 1, MoveTarget::OneEnemy, Effect::None, 0.8f);
        case MoveID::MiniSledge: return buildBasic(moveID, "Mini Sledge", 30, 1, MoveTarget::OneEnemy, Effect::DefenseScalingDamage, 1.5f);

        case MoveID::VinceBasic: return buildBasic(moveID, "Basic Attack", 0, 2, MoveTarget::OneEnemy, Effect::SlowEnemy, 0.8f); // does about 70 damage total to boss
        case MoveID::Sledgehammer: return buildSkill(moveID, "Sledgehammer", 35, 1, MoveTarget::OneEnemy, Effect::DefenseScalingDamage, 1.5f); // does about 130 damage to boss
        case MoveID::HoldTheLine: return buildSupport(moveID, "Hold The Line", 80, MoveTarget::AllAllies, Effect::HoldLineShield);

        // Young Joe
        case MoveID::Windmill: return buildBasic(moveID, "Windmill", 10, 3, MoveTarget::OneEnemy, Effect::None, 0.4f);

        // Young Justin
        case MoveID::Tackle: return buildBasic(moveID, "Tackle", 7, 1, MoveTarget::OneEnemy, Effect::None, 0.9f);

        // --- TONY ---
        case MoveID::TonyP1Basic: return buildBasic(moveID, "Basic Attack", 0, 2, MoveTarget::OneEnemy, Effect::None, 1.0f); // 150-200 damage to characters
        case MoveID::PrecisionStrike: return buildSkill(moveID, "Precision Strike", 40, 1, MoveTarget::OneEnemy, Effect::PrecisionStrikeDebuff, 2.0f); // 150-200 damage to characters
        case MoveID::OlderBrother: return buildUlt(moveID, "Older Brother", 80, 1, MoveTarget::OneEnemy, Effect::StaminaStrip, 3.5f); // 300 damage to characters

        case MoveID::TonyP2Basic: return buildBasic(moveID, "Basic Attack", 0, 3, MoveTarget::OneEnemy, Effect::P2BasicDebuff, 1.0f); // 240 damage to characters
        case MoveID::Clutch: return buildSkill(moveID, "Clutch", 40, 1, MoveTarget::OneEnemy, Effect::ClutchGrab, 2.0f); // 150-200 damage to characters
        case MoveID::LightsOut: return buildUlt(moveID, "Lights Out", 100, 1, MoveTarget::OneEnemy, Effect::LightsOutStun, 4.0f); // 350 damage to characters

        default:
            std::cout << "ERROR: MoveID not found in Database!\n";
            return buildBasic(MoveID::Unknown, "Unknown Move", 0, 1);
    }
}

std::string getMoveName(MoveID moveID) {
    return getMove(moveID).name;
}