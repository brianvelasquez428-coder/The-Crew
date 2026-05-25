#include "CombatUI.h" 
#include "CombatEngine.h" 
#include "AbilityDatabase.h"
#include "MoveDatabase.h" 
#include "../Systems/Logger.h" 
#include "raylib.h" 
#include <string> 
#include <algorithm>
#include <map> // <--- Ensure this is here!

// <--- Add the extern promise here!
extern std::map<ActorID, Texture2D> globalSprites; 

// --- ANIMATION TRACKERS ---
Entity* animAttacker = nullptr;
Vector2 animAttackerPos = {0,0};
Entity* animTarget = nullptr;
Vector2 animTargetOffset = {0,0};
Entity* animDying = nullptr;     
int deathFadeAlpha = 255;        

Vector2 GetBasePos(Entity* e, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam) {
    // Shifted down to Y = 100 to leave room for Damage Popups!
    Vector2 pPos[3] = { {200, 100}, {350, 240}, {200, 380} };
    Vector2 ePos[3] = { {1600, 100}, {1450, 240}, {1600, 380} };
    
    for (int i = 0; i < pTeam.size(); i++) {
        if (pTeam[i] == e) return pPos[i];
    }
    for (int i = 0; i < eTeam.size(); i++) {
        if (eTeam[i] == e) return ePos[i];
    }
    return {0, 0};
}

static std::string getTargetText(MoveTarget t) {
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

static std::string getEffectText(Effect e) {
    switch(e) {
        case Effect::None: return "No special effect.";
        case Effect::RestoreStamina: return "Restores 25 Stamina.";
        case Effect::DefenseScalingDamage: return "Uses user's Defense stat to calculate damage.";
        case Effect::ApplyBleed: return "Applies Bleed (5 DMG/turn) for 3 turns.";
        case Effect::IgnoreDefense: return "Ignores 50% of the target's Defense.";
        case Effect::DefenseBuff40: return "Increases Defense by 40% and adds 1 Hit Nullification.";
        case Effect::HealAndCleanse: return "Heals 40% HP and cures all status conditions.";
        case Effect::LowerPriority: return "Reduces target's Speed by 15 for 2 turns.";
        case Effect::StrikeDefenseDebuff: return "Reduces target's Defense by 40% for 2 turns.";
        case Effect::StrikeUlt: return "Guaranteed Hit & Crit. Consumes stacks for massive damage.";
        case Effect::SlowEnemy: return "Reduces target's Speed by 20 for 2 turns.";
        case Effect::HoldLineShield: return "Applies a Shield.";
        case Effect::PrecisionStrikeDebuff: return "Reduces target's DEF by 15 and BIQ by 10 for 3 turns.";
        case Effect::StaminaStrip: return "Halves target's Stamina Regeneration for 2 turns.";
        case Effect::P2BasicDebuff: return "Phase 2 Enhanced Basic Attack.";
        case Effect::ClutchGrab: return "Grabs target, preventing escape.";
        case Effect::LightsOutStun: return "Stuns the target for 1 turn.";
        default: return "Unknown effect.";
    }
}

bool DrawGUIButton(Rectangle rect, const char* text, int hotkey, bool disabled = false) {
    static double lastClickTime = 0; 
    Vector2 mousePos = GetMousePosition(); 
    bool isHovered = CheckCollisionPointRec(mousePos, rect);
    if (disabled) isHovered = false;
    
    DrawRectangleRec(rect, isHovered ? LIGHTGRAY : DARKGRAY); DrawRectangleLinesEx(rect, 2, WHITE); DrawText(text, rect.x + 20, rect.y + 15, 20, (isHovered ? BLACK : WHITE));
    
    if (!disabled && ((isHovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) || (hotkey != 0 && IsKeyReleased(hotkey)))) { 
        if (GetTime() - lastClickTime > 0.25) { lastClickTime = GetTime(); return true; } 
    } 
    return false;
}

void DrawBattleOverlay(std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, int momentum) {
    // 1. Draw Main UI Backgrounds
    DrawRectangle(0, 600, 1920, 480, DARKGRAY);
    DrawRectangleLines(0, 600, 1920, 480, WHITE);

    // Momentum Bar (Moved to Top Center of UI)
    DrawText("MOMENTUM:", 700, 615, 25, YELLOW);
    DrawRectangle(860, 620, 200, 15, BLACK);
    DrawRectangle(860, 620, momentum * 2, 15, YELLOW);

    // 2. Draw Player UI Profiles (Left Side)
    int uiY = 650;
    for (int i = 0; i < playerTeam.size(); i++) {
        Entity* p = playerTeam[i];
        if (!p) continue;
        
        // Profile Picture
        Rectangle profileRect = { 50, (float)uiY, 80, 80 };
        if (globalSprites.count(p->actorID)) {
            // Source cuts the 7x15 texture exactly in half to grab the top 7 pixels (Face/Shoulders)
            Rectangle sourceCrop = {0, 0, 7, 7}; 
            DrawTexturePro(globalSprites[p->actorID], sourceCrop, profileRect, {0,0}, 0.0f, p->isAlive ? WHITE : DARKGRAY);
            DrawRectangleLinesEx(profileRect, 2, WHITE);
        } else {
            // Fallback for characters without sprites
            DrawRectangleRec(profileRect, p->isAlive ? BLUE : DARKGRAY);
            DrawRectangleLinesEx(profileRect, 2, WHITE);
            DrawText(p->name.substr(0, 3).c_str(), 65, uiY + 30, 20, WHITE); 
        }
        
        if (!p->isAlive) {
            DrawText("KNOCKED OUT", 150, uiY + 30, 20, RED);
            uiY += 120;
            continue;
        }

        // Name & Level
        DrawText(TextFormat("%s [Lv %d]", p->name.c_str(), p->level), 150, uiY, 20, WHITE);

        // HP Text & Stacked Bar
        int displayHP = p->currentHP + p->shieldHP;
        DrawText(TextFormat("HP: %d / %d", displayHP, p->maxHP), 150, uiY + 25, 18, GREEN);
        
        DrawRectangle(150, uiY + 45, 200, 15, BLACK);
        float hpPercent = (float)p->currentHP / p->maxHP;
        if (hpPercent > 1.0f) hpPercent = 1.0f;
        DrawRectangle(150, uiY + 45, hpPercent * 200, 15, GREEN);
        
        // Add Blue Shield to the Bar
        if (p->shieldHP > 0) {
            float shieldPercent = (float)p->shieldHP / p->maxHP;
            if (hpPercent + shieldPercent > 1.0f) shieldPercent = 1.0f - hpPercent;
            DrawRectangle(150 + (hpPercent * 200), uiY + 45, shieldPercent * 200, 15, SKYBLUE);
        }

        // Stamina Text & Bar
        DrawText(TextFormat("STM: %d / %d", p->currentStamina, p->maxStamina), 150, uiY + 65, 18, YELLOW);
        DrawRectangle(150, uiY + 85, 200, 15, BLACK);
        float stamPercent = (float)p->currentStamina / p->maxStamina;
        DrawRectangle(150, uiY + 85, stamPercent * 200, 15, YELLOW);

        // Status Icons (Text Placeholders)
        int statX = 370;
        int statY = uiY + 20;
        for (auto& s : p->activeStatuses) {
            std::string statStr = "UP"; Color statCol = GREEN;
            if (s.value < 0) { statStr = "DOWN"; statCol = RED; } // <--- Changed from DN to DOWN
            
            std::string label = "";
            if (s.type == StatusType::StatModifier) {
                if (s.targetStat == StatName::ATK) label = "ATK " + statStr;
                if (s.targetStat == StatName::DEF) label = "DEF " + statStr;
                if (s.targetStat == StatName::SPD) label = "SPD " + statStr;
                if (s.targetStat == StatName::BIQ) label = "BIQ " + statStr;
                if (s.targetStat == StatName::SIQ) label = "SIQ " + statStr;
            } else if (s.type == StatusType::Bleed) label = "BLEED";
            else if (s.type == StatusType::Stun) label = "STUN";
            else if (s.type == StatusType::Taunt) label = "TAUNT";

            if (label != "") {
                DrawText(label.c_str(), statX, statY, 15, statCol);
                statY += 20;
                if (statY > uiY + 80) { statY = uiY + 20; statX += 60; }
            }
        }
        uiY += 120;
    }

    // 3. Scrollable Combat Log (Right Side)
    static int combatLogScrollY = 0;
    Rectangle logRec = { 1300, 600, 620, 480 };
    DrawRectangleRec(logRec, Fade(BLACK, 0.5f));
    DrawRectangleLinesEx(logRec, 2, WHITE);
    DrawText("COMBAT LOG", 1320, 620, 25, LIGHTGRAY);
    
    // Scrolling logic
    if (CheckCollisionPointRec(GetMousePosition(), logRec)) {
        combatLogScrollY += GetMouseWheelMove() * 30;
        if (combatLogScrollY > 0) combatLogScrollY = 0;
    }

    BeginScissorMode(1300, 660, 620, 410);
    std::vector<std::string> logs = GameLog::GetMessages(); 
    int logY = 670 + combatLogScrollY;
    for (std::string& msg : logs) { 
        DrawText(msg.c_str(), 1320, logY, 20, WHITE); 
        logY += 30; 
    }
    EndScissorMode();

    // 4. Draw Sprites in V-Formation
    for (Entity* p : playerTeam) {
        if (!p->isAlive && p != animDying && p != animTarget) continue;
        
        Color tint = WHITE; Color textColor = WHITE;
        if (p == animDying) { 
            float a = deathFadeAlpha / 255.0f;
            tint = Fade(WHITE, a); textColor = Fade(textColor, a); 
        }
        
        Vector2 drawPos = GetBasePos(p, playerTeam, enemyTeam);
        if (p == animAttacker) drawPos = animAttackerPos;
        if (p == animTarget) { drawPos.x += animTargetOffset.x; drawPos.y += animTargetOffset.y; }
        
        // NEW SIZE: 70x150
        Rectangle destRect = {drawPos.x, drawPos.y, 70, 150};
        if (globalSprites.count(p->actorID)) {
            Rectangle fullSource = {0, 0, 7, 15};
            DrawTexturePro(globalSprites[p->actorID], fullSource, destRect, {0,0}, 0.0f, tint);
        } else {
            DrawRectangle(destRect.x, destRect.y, destRect.width, destRect.height, BLUE); 
        }
        
        // X shifted to -10 to center the name over the thinner 70px sprite
        DrawText(p->name.c_str(), drawPos.x - 10, drawPos.y - 30, 25, textColor);
    }
    
    for (Entity* e : enemyTeam) {
        if (!e->isAlive && e != animDying && e != animTarget) continue;
        
        Color hpColor = RED; Color bodyColor = RED; Color textColor = WHITE; Color barBgColor = BLACK;
        Color tint = WHITE; // <--- ADDED THIS HERE!
        
        if (e == animDying) { 
            float a = deathFadeAlpha / 255.0f;
            hpColor = Fade(hpColor, a); bodyColor = Fade(bodyColor, a);
            textColor = Fade(textColor, a); barBgColor = Fade(barBgColor, a);
            tint = Fade(WHITE, a); // <--- FADES THE SPRITE WHEN DYING
        }
        
        Vector2 drawPos = GetBasePos(e, playerTeam, enemyTeam);
        if (e == animAttacker) drawPos = animAttackerPos;
        if (e == animTarget) { drawPos.x += animTargetOffset.x; drawPos.y += animTargetOffset.y; }
        
        // NEW SIZE: 70x150
        Rectangle destRect = {drawPos.x, drawPos.y, 70, 150};
        if (globalSprites.count(e->actorID)) {
            Rectangle fullSource = {0, 0, 7, 15};
            DrawTexturePro(globalSprites[e->actorID], fullSource, destRect, {0,0}, 0.0f, tint); // Now it knows what tint is!
        } else {
            DrawRectangle(destRect.x, destRect.y, destRect.width, destRect.height, bodyColor); 
        }
        
        // X shifted to -10 to center the name over the thinner 70px sprite
        DrawText(e->name.c_str(), drawPos.x - 10, drawPos.y - 30, 25, textColor);
        
        // Enemy HP and Shield Bars centered under the 70px sprite
        if (e->shieldHP > 0) {
            int shieldWidth = (e->shieldHP * 140) / e->maxHP;
            if (shieldWidth > 140) shieldWidth = 140; 
            DrawRectangle(drawPos.x - 35, drawPos.y + 155, 140, 10, barBgColor);
            DrawRectangle(drawPos.x - 35, drawPos.y + 155, shieldWidth, 10, SKYBLUE);
        }
        
        // HP Bar and Fraction Text centered under the 70px sprite
        DrawRectangle(drawPos.x - 35, drawPos.y + 165, 140, 15, barBgColor); 
        DrawRectangle(drawPos.x - 35, drawPos.y + 165, (e->currentHP * 140) / e->maxHP, 15, hpColor);
        DrawText(TextFormat("%d / %d", e->currentHP, e->maxHP), drawPos.x - 35, drawPos.y + 185, 20, textColor);
    }
}

void AnimateApproach(Entity* attacker, Entity* target, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum) {
    Vector2 start;
    if (animAttacker == attacker) { start = animAttackerPos; } 
    else { start = GetBasePos(attacker, pTeam, eTeam); }
    
    Vector2 end = GetBasePos(target, pTeam, eTeam);
    if (start.x < end.x) end.x -= 120; else end.x += 120; 
    
    animAttacker = attacker;
    for (int i=0; i<=10; i++) { 
        float t = i / 10.0f;
        animAttackerPos.x = start.x + (end.x - start.x) * t;
        animAttackerPos.y = start.y + (end.y - start.y) * t;
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(pTeam, eTeam, momentum); EndDrawing();
    }
}

void AnimateReturn(Entity* attacker, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum) {
    Vector2 start = animAttackerPos;
    Vector2 end = GetBasePos(attacker, pTeam, eTeam);
    for (int i=0; i<=10; i++) { 
        float t = i / 10.0f;
        animAttackerPos.x = start.x + (end.x - start.x) * t;
        animAttackerPos.y = start.y + (end.y - start.y) * t;
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(pTeam, eTeam, momentum); EndDrawing();
    }
    animAttacker = nullptr;
}

void AnimateHit(Entity* target, int damage, bool isCrit, bool isDodge, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum, int durationFrames) {
    animTarget = target;
    std::string popupText; Color popColor;
    if (isDodge) { popupText = "DODGE!"; popColor = GRAY; }
    else if (damage < 0) { popupText = "SUPPORT"; popColor = GREEN; } 
    else { 
        popupText = std::to_string(damage); 
        if (isCrit) { popupText += " CRIT!"; popColor = YELLOW; } else popColor = WHITE;
    }
    Vector2 baseT = GetBasePos(target, pTeam, eTeam);
    for (int i=0; i<durationFrames; i++) { 
        if (!isDodge && damage >= 0 && i < 15) { 
            animTargetOffset.x = (GetRandomValue(0, 20) - 10);
            animTargetOffset.y = (GetRandomValue(0, 20) - 10);
        } else {
            animTargetOffset = {0,0}; 
        }
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(pTeam, eTeam, momentum);
        int slideUp = (i < 20) ? (i*2) : 40;
        DrawText(popupText.c_str(), baseT.x + 20, baseT.y - 40 - slideUp, isCrit? 40 : 30, popColor);
        EndDrawing();
    }
    animTargetOffset = {0,0}; animTarget = nullptr;
}

void AnimateDeath(Entity* target, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum) {
    animDying = target;
    for (int i = 255; i >= 0; i -= 8) { 
        deathFadeAlpha = i;
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(pTeam, eTeam, momentum); EndDrawing();
    }
    animDying = nullptr;
    deathFadeAlpha = 255; 
}

void AnimateSupport(Entity* caster, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum) {
    animAttacker = caster;
    Vector2 base = GetBasePos(caster, pTeam, eTeam);
    for (int i=0; i<20; i++) { 
        animAttackerPos = base;
        animAttackerPos.y -= (i < 10) ? i*2 : (20-i)*2; 
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(pTeam, eTeam, momentum); EndDrawing();
    }
    animAttacker = nullptr;
}

void AnimatePopupsForEntity(Entity* target, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum) {
    if (target->pendingPopups.empty()) return;

    bool isPlayer = false;
    for (auto* p : pTeam) if (p == target) isPlayer = true;

    Vector2 baseT = GetBasePos(target, pTeam, eTeam);

    for (auto& popup : target->pendingPopups) {
        float offsetX = popup.isBuff ? (isPlayer ? 110 : -60) : (isPlayer ? -60 : 110);              
        Color pCol = popup.isBuff ? GREEN : RED; 

        // Increased from 30 to 60 frames!
        for (int i=0; i<30; i++) {
            BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(pTeam, eTeam, momentum);

            int slideUp = i; // Rises slower
            float alpha = 1.0f;
            if (i > 40) alpha = 1.0f - ((i - 40) / 20.0f); // Fades out smoothly at the very end

            DrawText(popup.text.c_str(), baseT.x + offsetX, baseT.y + 60 - slideUp, 20, Fade(pCol, alpha));

            EndDrawing();
        }
    }
    target->pendingPopups.clear();
}

void AnimatePhaseTransition(int phaseNum, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum) {
    for(int i = 0; i < 20; i++) {
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(pTeam, eTeam, momentum); EndDrawing();
    }
    double entryTime = GetTime();
    while (!WindowShouldClose()) {
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(pTeam, eTeam, momentum);

        DrawRectangle(0, 0, 1920, 1080, Fade(BLACK, 0.7f));
        std::string text = "PHASE " + std::to_string(phaseNum);
        DrawText(text.c_str(), 1920/2 - MeasureText(text.c_str(), 100)/2, 400, 100, RED);
        DrawText("[ CLICK MOUSE OR PRESS ENTER TO CONTINUE ]", 1920/2 - MeasureText("[ CLICK MOUSE OR PRESS ENTER TO CONTINUE ]", 30)/2, 600, 30, LIGHTGRAY);

        EndDrawing();
        if (GetTime() - entryTime > 0.5) {
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsKeyReleased(KEY_ENTER)) break;
        }
    }
    double exitTime = GetTime();
    while (GetTime() - exitTime < 0.2 && !WindowShouldClose()) {
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(pTeam, eTeam, momentum); EndDrawing();
    }
}

void pauseForPlayer(bool wasPlayerTurn, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, int momentum) {
    double entryTime = GetTime(); 
    while (!WindowShouldClose()) {
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(playerTeam, enemyTeam, momentum);
        
        // Match the center console dimensions exactly
        DrawRectangle(550, 650, 700, 400, Fade(DARKPURPLE, 0.6f)); 
        DrawRectangleLines(550, 650, 700, 400, PURPLE);
        
        // Centered prompt
        DrawText("[ CLICK MOUSE OR PRESS ENTER TO CONTINUE ]", 580, 830, 25, YELLOW);
        
        EndDrawing();
        if (GetTime() - entryTime > 0.25) {
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsKeyReleased(KEY_ENTER)) break;
        }
    }
    double exitTime = GetTime();
    while (GetTime() - exitTime < 0.2 && !WindowShouldClose()) {
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(playerTeam, enemyTeam, momentum); EndDrawing();
    }
}

void printCharacterStats(Entity* character) { GameLog::Add("--- " + character->name + "'s Turn ---"); }

void handleStunnedCharacter(Entity* character, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, int momentum) {
    GameLog::Add(">>> " + character->name + " is stunned and loses their turn! <<<"); character->removeStatusByType(StatusType::Stun); pauseForPlayer(false, playerTeam, enemyTeam, momentum);
}

std::vector<Entity*> requestPlayerTargets(Entity* attacker, Move selectedMove, std::vector<Entity*>& enemyTeam, std::vector<Entity*>& playerTeam) {
    std::vector<Entity*> selectedTargets;
    if (selectedMove.target == MoveTarget::Self) { selectedTargets.push_back(attacker); return selectedTargets; }
    if (selectedMove.target == MoveTarget::AllEnemies) { for (Entity* e : enemyTeam) if (e->isAlive) selectedTargets.push_back(e); return selectedTargets; }
    if (selectedMove.target == MoveTarget::AllAllies) { for (Entity* p : playerTeam) if (p->isAlive) selectedTargets.push_back(p); return selectedTargets; }
    
    bool targetsEnemies = (selectedMove.target == MoveTarget::OneEnemy || selectedMove.target == MoveTarget::TwoEnemies);
    int targetsNeeded = (selectedMove.target == MoveTarget::TwoEnemies || selectedMove.target == MoveTarget::TwoAllies) ? 2 : 1;
    std::vector<Entity*>& validPool = targetsEnemies ? enemyTeam : playerTeam;
    int aliveCount = 0; for (Entity* t : validPool) if (t->isAlive) aliveCount++;
    if (targetsNeeded > aliveCount) targetsNeeded = aliveCount; if (targetsNeeded == 0) return selectedTargets;
    
    while (selectedTargets.size() < targetsNeeded && !WindowShouldClose()) {
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(playerTeam, enemyTeam, 0);
        
        DrawRectangle(500, 600, 800, 480, Fade(DARKPURPLE, 0.6f)); DrawRectangleLines(500, 600, 800, 480, PURPLE);
        DrawText(TextFormat("SELECT TARGET(S) FOR: %s (%d needed)", selectedMove.name.c_str(), targetsNeeded - selectedTargets.size()), 530, 630, 30, WHITE);
        DrawText("[ HOVER OVER A TARGET AND CLICK, OR PRESS 'B' TO CANCEL ]", 530, 700, 20, LIGHTGRAY);
        
        for (Entity* e : validPool) {
            if (e->isAlive) {
                Vector2 pos = GetBasePos(e, playerTeam, enemyTeam);
                // Shrunk to wrap tightly around the 70x150 sprite
                Rectangle targetBox = { pos.x - 20, pos.y - 20, 110, 200 };
                
                bool alreadySelected = (std::find(selectedTargets.begin(), selectedTargets.end(), e) != selectedTargets.end());
                if (alreadySelected) { DrawRectangleLinesEx(targetBox, 3, GREEN); DrawText("SELECTED", targetBox.x, targetBox.y - 30, 20, GREEN); } 
                else {
                    if (CheckCollisionPointRec(GetMousePosition(), targetBox)) {
                        DrawRectangleLinesEx(targetBox, 3, YELLOW); DrawText("TARGET", targetBox.x, targetBox.y - 30, 20, YELLOW);
                        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) selectedTargets.push_back(e);
                    }
                }
            }
        }
        EndDrawing();
        if (IsKeyReleased(KEY_B)) { selectedTargets.clear(); return selectedTargets; } 
    }
    return selectedTargets;
}

void manageParty(std::vector<Entity*>& masterRoster, std::vector<Entity*>& activeParty) { }

void executePlayerTurn(Entity* character, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, std::vector<Entity*>& masterRoster, std::vector<TeamUpSkill>& masterTeamUps, int inventory[3], int& wallet, int& teamMomentum, std::vector<std::string>& previouslyActiveTeamUps, bool& battleIsActive, bool canFlee) {
    bool turnComplete = false; bool inItemMenu = false; bool inReviveMenu = false; bool inTeamUpMenu = false; bool inSwapMenu = false;
    int swapGroup = -1; int swapIndex = -1;
    
    bool showDetailsPopup = false; Move detailedMove; Entity* detailedEntity = nullptr; int detailsType = 0; int pendingMomentumCost = 0; 
    
    while (!turnComplete && battleIsActive && !WindowShouldClose()) {
        Move selectedMove; bool moveSelected = false;
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(playerTeam, enemyTeam, teamMomentum);
        
        DrawRectangle(550, 650, 700, 400, Fade(DARKBLUE, 0.5f)); 
        DrawRectangleLines(550, 650, 700, 400, WHITE);
        
        // 1. Only draw the main "WHAT WILL DO" text if we are NOT in a sub-menu
        bool isMainMenu = (!inItemMenu && !inReviveMenu && !inTeamUpMenu && !inSwapMenu);
        if (isMainMenu) {
            DrawText(TextFormat("WHAT WILL %s DO?", character->name.c_str()), 580, 670, 30, WHITE);
            DrawText("[ Right-Click a Sprite or Profile Picture to Inspect! ]", 580, 710, 18, LIGHTGRAY);
        }
        
        if (!showDetailsPopup) {
            // Check Enemy Sprite Clicks
            for (Entity* e : enemyTeam) {
                if (!e->isAlive) continue; 
                Vector2 pos = GetBasePos(e, playerTeam, enemyTeam);
                // Updated Hitbox
                if (CheckCollisionPointRec(GetMousePosition(), {pos.x - 20, pos.y - 20, 110, 200}) && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) { detailedEntity = e; detailsType = 5; showDetailsPopup = true; }
            }
            
            // Check Player Sprite AND Profile Clicks
            for (int i = 0; i < playerTeam.size(); i++) {
                Entity* p = playerTeam[i];
                if (!p->isAlive) continue; 
                Vector2 pos = GetBasePos(p, playerTeam, enemyTeam);
                
                // Updated Hitbox
                if (CheckCollisionPointRec(GetMousePosition(), {pos.x - 20, pos.y - 20, 110, 200}) && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) { detailedEntity = p; detailsType = 5; showDetailsPopup = true; }
                
                // Profile Picture Click (DO NOT CHANGE)
                if (CheckCollisionPointRec(GetMousePosition(), {50, 650.0f + (i * 120), 80, 80}) && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) { detailedEntity = p; detailsType = 5; showDetailsPopup = true; }
            }
        }
        
        if (isMainMenu) {
            int startX = 580; int startY = 750; int buttonKeys[4] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR};
            for (int i = 0; i < character->combatMenu.size(); i++) {
                Rectangle btn = { (float)startX, (float)startY + (i * 50), 300, 40 };
                if (DrawGUIButton(btn, (std::to_string(i+1) + ". " + character->combatMenu[i].name + " [" + std::to_string(character->combatMenu[i].staminaCost) + "]").c_str(), buttonKeys[i], showDetailsPopup)) {
                    if (character->currentStamina >= character->combatMenu[i].staminaCost) { selectedMove = character->combatMenu[i]; moveSelected = true; } else GameLog::Add("[!] Not enough stamina!");
                }
                if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && !showDetailsPopup) { detailedMove = character->combatMenu[i]; detailsType = 1; showDetailsPopup = true; }
            }
            
            if (DrawGUIButton({900, 750, 250, 40}, "5. Backpack", KEY_FIVE, showDetailsPopup)) inItemMenu = true;
            if (DrawGUIButton({900, 800, 250, 40}, "6. Team-Up", KEY_SIX, showDetailsPopup)) inTeamUpMenu = true;
            if (canFlee && DrawGUIButton({900, 850, 250, 40}, "7. Retreat", KEY_SEVEN, showDetailsPopup)) { GameLog::Add(">>> FELL BACK TO HIDEOUT <<<"); battleIsActive = false; turnComplete = true; }
            
            int nextBtnNum = 8;
            if (!character->altCombatMenu.empty()) {
                if (DrawGUIButton({900, 900, 250, 40}, TextFormat("%d. Stance (%s)", nextBtnNum, character->isAltStance ? "Strike" : "Support"), (nextBtnNum == 8 ? KEY_EIGHT : KEY_NINE), showDetailsPopup)) { 
                    character->toggleStance(); GameLog::Add(character->name + " shifted their combat stance!"); 
                }
                nextBtnNum++;
            }
            
            if (DrawGUIButton({900, 950, 250, 40}, TextFormat("%d. Swap Crew", nextBtnNum), (nextBtnNum == 8 ? KEY_EIGHT : KEY_NINE), showDetailsPopup)) {
                inSwapMenu = true;
            }
        } 
        else if (inItemMenu && !inReviveMenu) {
            // Text Shifted up to 670 to match main menu
            DrawText("BACKPACK:", 580, 670, 30, WHITE); 
            DrawText("[ Select an item to use ]", 580, 710, 18, LIGHTGRAY);
            bool usedItem = false;
            
            if (DrawGUIButton({580, 750, 300, 40}, TextFormat("1. Bandage (x%d)", inventory[0]), KEY_ONE, showDetailsPopup)) { if (inventory[0] > 0) { inventory[0]--; character->currentHP += (character->maxHP * 0.3); if (character->currentHP > character->maxHP) character->currentHP = character->maxHP; GameLog::Add(character->name + " used a Bandage!"); usedItem = true; } else GameLog::Add("Out of Bandages!"); }
            if (DrawGUIButton({580, 800, 300, 40}, TextFormat("2. Energy Drink (x%d)", inventory[1]), KEY_TWO, showDetailsPopup)) { if (inventory[1] > 0) { inventory[1]--; character->currentStamina += 40; if (character->currentStamina > 200) character->currentStamina = 200; GameLog::Add(character->name + " drank an Energy Drink!"); usedItem = true; } else GameLog::Add("Out of Energy Drinks!"); }
            if (DrawGUIButton({580, 850, 300, 40}, TextFormat("3. Revive (x%d)", inventory[2]), KEY_THREE, showDetailsPopup)) { if (inventory[2] > 0) { bool anyoneDead = false; for (Entity* p : playerTeam) if (!p->isAlive) anyoneDead = true; if (anyoneDead) { inItemMenu = false; inReviveMenu = true; } else GameLog::Add("Everyone is already alive!"); } else GameLog::Add("Out of Revives!"); }
            
            if (DrawGUIButton({900, 750, 200, 40}, "4. Back [B]", KEY_FOUR, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) inItemMenu = false;
            
            if (usedItem) turnComplete = true; 
        }
        else if (inReviveMenu) {
            DrawText("WHO TO REVIVE?", 580, 670, 30, WHITE);
            int startY = 750; int btnIndex = 0; int buttonKeys[3] = {KEY_ONE, KEY_TWO, KEY_THREE};
            
            for (Entity* p : playerTeam) {
                if (!p->isAlive) {
                    if (DrawGUIButton({ 580.0f, (float)startY + (btnIndex * 50), 300.0f, 40.0f }, (std::to_string(btnIndex + 1) + ". " + p->name).c_str(), buttonKeys[btnIndex], showDetailsPopup)) { inventory[2]--; p->isAlive = true; p->currentHP = p->maxHP / 2; GameLog::Add(character->name + " used a Revive on " + p->name + "!"); turnComplete = true; break; }
                    btnIndex++;
                }
            }
            if (DrawGUIButton({900, 750, 200, 40}, "4. Cancel [B]", KEY_FOUR, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) { inReviveMenu = false; inItemMenu = true; }
        }
        else if (inTeamUpMenu) {
            DrawText("TEAM-UPS:", 580, 670, 30, WHITE);
            std::vector<TeamUpSkill> activeCombos = getActiveTeamUps(playerTeam, masterTeamUps);
            
            if (activeCombos.empty()) DrawText("No Team-Ups available.", 580, 750, 20, RED); 
            else {
                int btnIndex = 0; int buttonKeys[4] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR};
                for (TeamUpSkill& combo : activeCombos) {
                    // Capped button width to 350 so it doesn't touch the right edge
                    Rectangle btn = { 580.0f, 750.0f + (btnIndex * 50), 350.0f, 40.0f }; 
                    if (DrawGUIButton(btn, (std::to_string(btnIndex + 1) + ". " + combo.name + " [" + std::to_string(combo.momentumCost) + "%]").c_str(), buttonKeys[btnIndex], showDetailsPopup)) { if (teamMomentum >= combo.momentumCost) { selectedMove = combo.moveData; pendingMomentumCost = combo.momentumCost; moveSelected = true; } else GameLog::Add("[!] Not enough Momentum!"); }
                    btnIndex++;
                }
            }
            
            // Shifted Cancel button to 1000 so it sits perfectly in the gap
            if (DrawGUIButton({1000, 750, 200, 40}, "0. Cancel [B]", KEY_ZERO, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) inTeamUpMenu = false;
        }
        else if (inSwapMenu) {
            DrawText("SWAP CREW MEMBERS", 580, 670, 30, WHITE);
            DrawText("[ Click two slots to swap. Turn is NOT consumed! ]", 580, 710, 18, YELLOW);
            
            std::vector<Entity*> tempActive(3, nullptr);
            for(size_t k = 0; k < playerTeam.size() && k < 3; k++) tempActive[k] = playerTeam[k];
            
            std::vector<Entity*> tempReserves(3, nullptr);
            for(size_t k = 0; k < masterRoster.size() && k < 3; k++) tempReserves[k] = masterRoster[k];
            
            auto getEntity = [&](int g, int i) -> Entity* {
                if (g == 0) return tempActive[i];
                if (g == 1) return tempReserves[i];
                return nullptr;
            };
            
            auto DrawSlot = [&](Rectangle r, int g, int i) {
                Entity* e = getEntity(g, i);
                std::string label = e ? TextFormat("%s [HP: %d]", e->name.c_str(), e->currentHP) : "[ EMPTY ]";
                
                if (DrawGUIButton(r, label.c_str(), 0, showDetailsPopup)) {
                    if (swapGroup == -1) { 
                        if (e != nullptr) { swapGroup = g; swapIndex = i; } 
                    } else { 
                        Entity* temp = getEntity(swapGroup, swapIndex);
                        
                        if (swapGroup == 0) tempActive[swapIndex] = getEntity(g, i);
                        else tempReserves[swapIndex] = getEntity(g, i);
                        
                        if (g == 0) tempActive[i] = temp;
                        else tempReserves[i] = temp;
                        
                        playerTeam.clear();
                        for (auto* ent : tempActive) if (ent != nullptr) playerTeam.push_back(ent);
                        
                        masterRoster.clear();
                        for (auto* ent : tempReserves) if (ent != nullptr) masterRoster.push_back(ent);
                        
                        swapGroup = -1; swapIndex = -1;
                    }
                }
                // Highlight the selected slot
                if (swapGroup == g && swapIndex == i) DrawRectangleLinesEx(r, 3, ORANGE);
            };

            DrawText("ACTIVE:", 580, 765, 20, GREEN);
            // Shifted active slots to 580
            for (int i=0; i < 3; i++) DrawSlot({580, 790.0f + (i * 45), 200, 40}, 0, i);

            // Shifted reserve slots to 800 (Sitting right next to Active)
            DrawText("RESERVES:", 800, 765, 20, GOLD);
            for (int i=0; i < 3; i++) DrawSlot({800, 790.0f + (i * 45), 200, 40}, 1, i);

            // Shifted Back button to 1020 (Sitting to the right of Reserves)
            if (DrawGUIButton({1020, 790, 200, 40}, "0. Back [B]", KEY_ZERO, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) {
                inSwapMenu = false; swapGroup = -1; swapIndex = -1;
            }
        }

        if (showDetailsPopup) {
            DrawRectangle(0, 0, 1920, 1080, Fade(BLACK, 0.95f));
            DrawRectangle(500, 300, 920, 480, DARKGRAY); DrawRectangleLinesEx({500, 300, 920, 480}, 4, WHITE);
            
            // --- FIX: RESTORED THE MOVE RENDERING BLOCK ---
            if (detailsType == 1) {
                DrawText(detailedMove.name.c_str(), 550, 350, 50, GOLD); 
                DrawText(TextFormat("TYPE: %s", getCategoryName(detailedMove.category).c_str()), 550, 420, 30, LIGHTGRAY);
                
                if (detailedMove.category == MoveCategory::TeamUp) DrawText(TextFormat("MOMENTUM COST: %d%%", detailedMove.staminaCost), 550, 470, 30, YELLOW);
                else DrawText(TextFormat("STAMINA COST: %d", detailedMove.staminaCost), 550, 470, 30, GREEN);
                
                DrawText(TextFormat("HITS: %d", detailedMove.hitCount), 550, 520, 30, WHITE);
                DrawText(TextFormat("POWER MULTIPLIER: %.1fx", detailedMove.powerMultiplier), 550, 570, 30, ORANGE);
                DrawText(TextFormat("TARGETS: %s", getTargetText(detailedMove.target).c_str()), 550, 640, 30, SKYBLUE);
                DrawText(TextFormat("EFFECT: %s", getEffectText(detailedMove.effect).c_str()), 550, 690, 30, PURPLE);
            } 
            else if (detailsType == 5 && detailedEntity != nullptr) {
                DrawText(TextFormat("INSPECTING: %s", detailedEntity->name.c_str()), 550, 350, 50, GOLD);
                
                // --- NEW: STATS DISPLAY ---
                DrawText(TextFormat("HP: %d/%d  |  ATK: %d  |  DEF: %d  |  SPD: %d", 
                    detailedEntity->currentHP, detailedEntity->maxHP,
                    detailedEntity->currentAttack, detailedEntity->currentDefense, 
                    detailedEntity->currentSpeed), 550, 410, 20, GREEN);
                
                DrawText(TextFormat("STM: %d/%d  |  BIQ: %d  |  SIQ: %d", 
                    detailedEntity->currentStamina, detailedEntity->maxStamina, 
                    detailedEntity->currentBIQ, detailedEntity->currentSIQ), 550, 440, 20, GREEN);
                
                int dY = 490; // Shifted downwards to accommodate stats
                DrawText("NATURAL ABILITY:", 550, dY, 25, SKYBLUE); dY += 30; 
                if (detailedEntity->naturalAbility != PassiveID::None) {
                    DrawText(TextFormat("- %s: %s", getPassiveName(detailedEntity->naturalAbility).c_str(), getPassiveDescription(detailedEntity->naturalAbility).c_str()), 570, dY, 20, LIGHTGRAY); dY += 40;
                }
                
                DrawText("PASSIVE ABILITIES:", 550, dY, 25, GREEN); dY += 30; 
                for(auto& p : detailedEntity->passiveAbilities) { 
                    DrawText(TextFormat("- %s: %s", getPassiveName(p).c_str(), getPassiveDescription(p).c_str()), 570, dY, 20, LIGHTGRAY); dY += 30; 
                } dY += 10;
                
                DrawText("ACTIVE ABILITIES:", 550, dY, 25, ORANGE); dY += 30; 
                for(auto& a : detailedEntity->activeAbilities) { 
                    DrawText(TextFormat("- %s: %s", getActiveName(a).c_str(), getActiveDescription(a).c_str()), 570, dY, 20, LIGHTGRAY); dY += 30; 
                }
            }
            if (DrawGUIButton({550, 800, 300, 60}, "Close Info [B]", 0) || IsKeyReleased(KEY_B)) showDetailsPopup = false;
        }
        EndDrawing();
        
        if (moveSelected && !showDetailsPopup) { 
            std::vector<Entity*> targets = requestPlayerTargets(character, selectedMove, enemyTeam, playerTeam);
            if (!targets.empty()) {
                if (selectedMove.category == MoveCategory::TeamUp) { teamMomentum -= pendingMomentumCost; GameLog::Add(">>> TEAM-UP INITIATED: " + selectedMove.name + " ! <<<"); } 
                else { character->useStamina(selectedMove.staminaCost); GameLog::Add(character->name + " used " + selectedMove.name + "!"); }
                
                executeCombatRoutine(character, selectedMove, targets, playerTeam, enemyTeam, wallet, teamMomentum); 
                turnComplete = true;
            } else { moveSelected = false; pendingMomentumCost = 0; }
        }
    }
}

bool checkBattleEnd(std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, std::vector<Entity*>& masterRoster) {
    bool allEnemiesDead = true; for (Entity* enemy : enemyTeam) if (enemy->isAlive) allEnemiesDead = false;
    bool allPlayersDead = true; for (Entity* player : playerTeam) if (player->isAlive) allPlayersDead = false;
         
    if (allEnemiesDead || allPlayersDead) {
        if (allEnemiesDead) GameLog::Add("VICTORY!"); if (allPlayersDead) GameLog::Add("DEFEAT...");
                 
        for(int i = 0; i < 60; i++) { 
             BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(playerTeam, enemyTeam, 0); EndDrawing();
        }
        while (!WindowShouldClose()) {
            BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(playerTeam, enemyTeam, 0);
             
            DrawRectangle(0, 0, 1920, 1080, Fade(BLACK, 0.7f));
            if (allEnemiesDead) DrawText("VICTORY!", 1920/2 - MeasureText("VICTORY!", 100)/2, 400, 100, GREEN);
            if (allPlayersDead) DrawText("DEFEAT...", 1920/2 - MeasureText("DEFEAT...", 100)/2, 400, 100, RED);
            DrawText("[ CLICK MOUSE OR PRESS ENTER TO RETURN TO HIDEOUT ]", 1920/2 - MeasureText("[ CLICK MOUSE OR PRESS ENTER TO RETURN TO HIDEOUT ]", 30)/2, 600, 30, LIGHTGRAY); EndDrawing();
                         
            static double vicTime = GetTime();
            if (GetTime() - vicTime > 0.5) {
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsKeyReleased(KEY_ENTER)) break;
            }
        }
        
        double exitTime = GetTime();
        while (GetTime() - exitTime < 0.3 && !WindowShouldClose()) {
            BeginDrawing(); ClearBackground(DARKBLUE); EndDrawing();
        }
        return true;
    } 
    return false;
}