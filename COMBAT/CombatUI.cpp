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
    
    std::vector<std::string> logs = GameLog::GetMessages(); 
    
    // ---> NEW: Dynamic Bound Clamping
    int totalLogHeight = logs.size() * 30; // Each line is 30 pixels
    int maxScroll = 0;
    if (totalLogHeight > 410) { 
        maxScroll = -(totalLogHeight - 410); // Calculates exactly how far down the log can go
    }
    
    // Automatically clamp the scroll value. If a new battle starts and the log shrinks, 
    // maxScroll becomes 0, and the old negative scroll value snaps right back to the top!
    if (combatLogScrollY < maxScroll) combatLogScrollY = maxScroll;
    if (combatLogScrollY > 0) combatLogScrollY = 0;

    // Scrolling logic
    if (CheckCollisionPointRec(GetMousePosition(), logRec)) {
        combatLogScrollY += GetMouseWheelMove() * 30;
        if (combatLogScrollY > 0) combatLogScrollY = 0;
        if (combatLogScrollY < maxScroll) combatLogScrollY = maxScroll;
    }

    BeginScissorMode(1300, 660, 620, 410);
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
    bool turnComplete = false; bool inItemMenu = false; bool inReviveMenu = false; bool inTeamUpMenu = false; bool inSwapMenu = false; bool inActiveMenu = false;
    ActiveID detailedActive = ActiveID::None; // For the right-click menu
    int swapGroup = -1; int swapIndex = -1; int detailedItem = 0; // For right-clicking backpack items
         
    bool showDetailsPopup = false; Move detailedMove; Entity* detailedEntity = nullptr; int detailsType = 0; int pendingMomentumCost = 0;
    
    // --- NEW: Add this variable to track which Team Up we right-clicked ---
    std::string detailedTeamUpName = "";
    
    while (!turnComplete && battleIsActive && !WindowShouldClose()) {
        Move selectedMove; bool moveSelected = false;
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(playerTeam, enemyTeam, teamMomentum);
        
        DrawRectangle(550, 650, 700, 400, Fade(DARKBLUE, 0.5f)); 
        DrawRectangleLines(550, 650, 700, 400, WHITE);
        
        // 1. Update this line so the main menu hides when you open the Active Skills menu!
        bool isMainMenu = (!inItemMenu && !inReviveMenu && !inTeamUpMenu && !inSwapMenu && !inActiveMenu);
        
        // 2. Keep your sprite clicking block right here at the top
        // 2. Keep your sprite clicking block right here at the top
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
                
                // Updated Hitbox for Sprite
                if (CheckCollisionPointRec(GetMousePosition(), {pos.x - 20, pos.y - 20, 110, 200}) && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) { detailedEntity = p; detailsType = 5; showDetailsPopup = true; }
                
                // Hitbox for Profile Picture
                if (CheckCollisionPointRec(GetMousePosition(), {50, 650.0f + (i * 120), 80, 80}) && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) { detailedEntity = p; detailsType = 5; showDetailsPopup = true; }
            }
        }
        
        // 3. Use the SINGLE unified scrolling block here
        if (isMainMenu) {
            DrawText(TextFormat("WHAT WILL %s DO?", character->name.c_str()), 580, 670, 30, WHITE);
            DrawText("[ Scroll to see more options | Right-Click to Inspect! ]", 580, 710, 18, LIGHTGRAY);
            
            // --- NEW: Scroll Logic ---
            static int mainMenuScrollY = 0;
            
            // Calculate how deep the menu needs to go
            int leftItems = character->combatMenu.size();
            int rightItems = 4; // Backpack, Team-Up, Retreat, Swap
            if (!character->activeAbilities.empty()) rightItems++;
            if (!character->altCombatMenu.empty()) rightItems++;
            
            int maxItems = std::max(leftItems, rightItems);
            int totalHeight = maxItems * 50; 
            int maxScroll = 0;
            if (totalHeight > 400) maxScroll = -(totalHeight - 400);

            if (mainMenuScrollY < maxScroll) mainMenuScrollY = maxScroll;
            if (mainMenuScrollY > 0) mainMenuScrollY = 0;
            
            if (CheckCollisionPointRec(GetMousePosition(), {550, 650, 700, 400})) {
                mainMenuScrollY += GetMouseWheelMove() * 30;
                if (mainMenuScrollY > 0) mainMenuScrollY = 0;
                if (mainMenuScrollY < maxScroll) mainMenuScrollY = maxScroll;
            }
            
            BeginScissorMode(550, 650, 700, 400);
            int leftY = 750 + mainMenuScrollY;
            int rightY = 750 + mainMenuScrollY;
            
            // --- LEFT COLUMN: Combat Moves ---
            int buttonKeys[4] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR};
            for (int i = 0; i < character->combatMenu.size(); i++) {
                Rectangle btn = { 580.0f, (float)leftY, 300, 40 };
                if (DrawGUIButton(btn, (std::to_string(i+1) + ". " + character->combatMenu[i].name + " [" + std::to_string(character->combatMenu[i].staminaCost) + "]").c_str(), buttonKeys[i], showDetailsPopup)) {
                    if (character->currentStamina >= character->combatMenu[i].staminaCost) { selectedMove = character->combatMenu[i]; moveSelected = true; } else GameLog::Add("[!] Not enough stamina!");
                }
                if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && !showDetailsPopup) { detailedMove = character->combatMenu[i]; detailsType = 1; showDetailsPopup = true; }
                leftY += 50;
            }
            
            // --- RIGHT COLUMN: Menu Actions ---
            if (DrawGUIButton({900, (float)rightY, 250, 40}, "5. Backpack", KEY_FIVE, showDetailsPopup)) inItemMenu = true;
            rightY += 50;
            
            if (DrawGUIButton({900, (float)rightY, 250, 40}, "6. Team-Up", KEY_SIX, showDetailsPopup)) inTeamUpMenu = true;
            rightY += 50;
            
            if (canFlee && DrawGUIButton({900, (float)rightY, 250, 40}, "7. Retreat", KEY_SEVEN, showDetailsPopup)) { GameLog::Add(">>> FELL BACK TO HIDEOUT <<<"); battleIsActive = false; turnComplete = true; }
            rightY += 50;
            
            int nextBtnNum = 8;
            if (!character->activeAbilities.empty()) {
                if (DrawGUIButton({900, (float)rightY, 250, 40}, TextFormat("%d. Active Skill", nextBtnNum), (nextBtnNum == 8 ? KEY_EIGHT : KEY_NINE), showDetailsPopup)) {
                    inActiveMenu = true;
                }
                rightY += 50;
                nextBtnNum++;
            }
            
            if (!character->altCombatMenu.empty()) {
                if (DrawGUIButton({900, (float)rightY, 250, 40}, TextFormat("%d. Stance (%s)", nextBtnNum, character->isAltStance ? "Strike" : "Support"), (nextBtnNum == 8 ? KEY_EIGHT : KEY_NINE), showDetailsPopup)) { 
                    character->toggleStance(); GameLog::Add(character->name + " shifted their combat stance!"); 
                }
                rightY += 50;
                nextBtnNum++;
            }
            
            if (DrawGUIButton({900, (float)rightY, 250, 40}, TextFormat("%d. Swap Crew", nextBtnNum), (nextBtnNum == 8 ? KEY_EIGHT : KEY_NINE), showDetailsPopup)) {
                inSwapMenu = true;
            }
            rightY += 50;
            
            EndScissorMode();
        } 
        else if (inItemMenu && !inReviveMenu) {
            DrawText("BACKPACK:", 580, 670, 30, WHITE); 
            DrawText("[ Select an item to use | Right-Click to Inspect ]", 580, 710, 18, LIGHTGRAY);
            bool usedItem = false;
            
            // 1. Bandage
            Rectangle btn1 = {580, 750, 300, 40};
            if (DrawGUIButton(btn1, TextFormat("1. Bandage (x%d)", inventory[0]), KEY_ONE, showDetailsPopup)) { if (inventory[0] > 0) { inventory[0]--; character->currentHP += (character->maxHP * 0.3); if (character->currentHP > character->maxHP) character->currentHP = character->maxHP; GameLog::Add(character->name + " used a Bandage!"); usedItem = true; } else GameLog::Add("Out of Bandages!"); }
            if (CheckCollisionPointRec(GetMousePosition(), btn1) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !showDetailsPopup) { detailedItem = 1; detailsType = 6; showDetailsPopup = true; }

            // 2. Energy Drink
            Rectangle btn2 = {580, 800, 300, 40};
            if (DrawGUIButton(btn2, TextFormat("2. Energy Drink (x%d)", inventory[1]), KEY_TWO, showDetailsPopup)) { if (inventory[1] > 0) { inventory[1]--; character->currentStamina += 40; if (character->currentStamina > 200) character->currentStamina = 200; GameLog::Add(character->name + " drank an Energy Drink!"); usedItem = true; } else GameLog::Add("Out of Energy Drinks!"); }
            if (CheckCollisionPointRec(GetMousePosition(), btn2) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !showDetailsPopup) { detailedItem = 2; detailsType = 6; showDetailsPopup = true; }

            // 3. Revive
            Rectangle btn3 = {580, 850, 300, 40};
            if (DrawGUIButton(btn3, TextFormat("3. Revive (x%d)", inventory[2]), KEY_THREE, showDetailsPopup)) { if (inventory[2] > 0) { bool anyoneDead = false; for (Entity* p : playerTeam) if (!p->isAlive) anyoneDead = true; if (anyoneDead) { inItemMenu = false; inReviveMenu = true; } else GameLog::Add("Everyone is already alive!"); } else GameLog::Add("Out of Revives!"); }
            if (CheckCollisionPointRec(GetMousePosition(), btn3) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !showDetailsPopup) { detailedItem = 3; detailsType = 6; showDetailsPopup = true; }
            
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
                    Rectangle btn = { 580.0f, 750.0f + (btnIndex * 50), 350.0f, 40.0f }; 
                    if (DrawGUIButton(btn, (std::to_string(btnIndex + 1) + ". " + combo.name + " [" + std::to_string(combo.momentumCost) + "%]").c_str(), buttonKeys[btnIndex], showDetailsPopup)) { 
                        if (teamMomentum >= combo.momentumCost) { selectedMove = combo.moveData; pendingMomentumCost = combo.momentumCost; moveSelected = true; } 
                        else GameLog::Add("[!] Not enough Momentum!"); 
                    }
                    
                    // --- NEW: Right-Click Inspection for Team Ups ---
                    if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !showDetailsPopup) {
                        detailedTeamUpName = combo.name;
                        detailsType = 2; // Type 2 is for Team Ups
                        showDetailsPopup = true;
                    }
                    // ------------------------------------------------

                    btnIndex++;
                }
            }
            
            // Shifted Cancel button to 1000 so it sits perfectly in the gap
            if (DrawGUIButton({1000, 750, 200, 40}, "0. Cancel [B]", KEY_ZERO, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) inTeamUpMenu = false;
        }
        else if (inActiveMenu) {
            DrawText("ACTIVE ABILITIES:", 580, 670, 30, ORANGE);
            if (character->usedActiveAbilityThisTurn) {
                DrawText("You already used an active ability this turn!", 580, 710, 20, RED);
            } else {
                for (int i = 0; i < character->activeAbilities.size(); i++) {
                    ActiveID aID = character->activeAbilities[i];
                    Rectangle btn = { 580.0f, 750.0f + (i * 50), 300.0f, 40.0f };
                    
                    if (DrawGUIButton(btn, (std::to_string(i+1) + ". " + getActiveName(aID)).c_str(), KEY_ONE + i, showDetailsPopup)) {
                        Entity* activeTarget = character; // Default to self
                        // If it's a debuff/attack, target the first alive enemy for simplicity
                        if (aID == ActiveID::Taunt || aID == ActiveID::BodyWork || aID == ActiveID::TheDeepEnd || aID == ActiveID::RealityCheck || aID == ActiveID::AnalyzeWeakness) {
                            for (Entity* e : enemyTeam) if (e->isAlive) { activeTarget = e; break; }
                        }
                        
                        executeAbility(aID, *character, *activeTarget);
                        character->usedActiveAbilityThisTurn = true;
                        inActiveMenu = false;
                    }
                    // Right Click to Inspect
                    if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !showDetailsPopup) {
                        detailedActive = aID; detailsType = 4; showDetailsPopup = true;
                    }
                }
            }
            if (DrawGUIButton({900, 750, 200, 40}, "0. Back [B]", KEY_ZERO, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) inActiveMenu = false;
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

                // ---> NEW: Right-Click Inspect for Mid-Battle Swapping <---
                if (CheckCollisionPointRec(GetMousePosition(), r) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !showDetailsPopup && e != nullptr) {
                    detailedEntity = e; 
                    detailsType = 5; // Triggers the full Character Inspection overlay
                    showDetailsPopup = true; 
                }
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
            // --- NEW: TEAM UP RENDER BLOCK ---
            else if (detailsType == 2) {
                for (TeamUpSkill& tu : masterTeamUps) {
                    if (tu.name == detailedTeamUpName) {
                        DrawText(tu.name.c_str(), 550, 350, 50, GOLD); 
                        DrawText(TextFormat("MOMENTUM COST: %d%%", tu.momentumCost), 550, 420, 30, YELLOW);
                        DrawText(TextFormat("HITS: %d", tu.moveData.hitCount), 550, 470, 30, WHITE); 
                        DrawText(TextFormat("POWER MULTIPLIER: %.1fx", tu.moveData.powerMultiplier), 550, 520, 30, ORANGE);
                        DrawText(TextFormat("TARGETS: %s", getTargetText(tu.moveData.target).c_str()), 550, 590, 30, SKYBLUE); 
                        DrawText(TextFormat("EFFECT: %s", getEffectText(tu.moveData.effect).c_str()), 550, 640, 30, PURPLE);
                        
                        // Parse out the required actors
                        std::string reqs = "CREW REQUIRED: ";
                        for (ActorID r : tu.requiredMembers) {
                            if (r == ActorID::Brian) reqs += "Brian  ";
                            else if (r == ActorID::Paul) reqs += "Paul  ";
                            else if (r == ActorID::Vince) reqs += "Vince  ";
                            else if (r == ActorID::Tony) reqs += "Tony  ";
                        }
                        DrawText(reqs.c_str(), 550, 690, 25, GREEN);
                        break;
                    }
                }
            }
            else if (detailsType == 4) {
                DrawText(getActiveName(detailedActive).c_str(), 550, 350, 50, GOLD);
                DrawText("TYPE: ACTIVE", 550, 420, 30, LIGHTGRAY);
                DrawText("EFFECT:", 550, 490, 25, ORANGE);
                DrawText(getActiveDescription(detailedActive).c_str(), 550, 530, 25, WHITE);
            }
            else if (detailsType == 5 && detailedEntity != nullptr) {
                static int inspectScrollY = 0;
                
                // --- NEW: Word Wrap Helper ---
                auto wrapText = [](const std::string& text, int maxChars) {
                    std::string wrapped = text;
                    int lastSpace = -1;
                    int lineStart = 0;
                    for (int i = 0; i < wrapped.length(); i++) {
                        if (wrapped[i] == ' ') lastSpace = i;
                        if (i - lineStart >= maxChars && lastSpace != -1) {
                            wrapped[lastSpace] = '\n';
                            lineStart = lastSpace + 1;
                            lastSpace = -1;
                        }
                    }
                    return wrapped;
                };

                // Helper to count lines for spacing
                auto countLines = [](const std::string& text) {
                    int lines = 1;
                    for(char c : text) if(c == '\n') lines++;
                    return lines;
                };

                // --- PRE-CALCULATE HEIGHT FOR SCROLL CLAMPING ---
                int predicted_dY = 530; // Base start for abilities
                if (detailedEntity->naturalAbility != PassiveID::None) {
                    std::string desc = wrapText(getPassiveDescription(detailedEntity->naturalAbility), 42);
                    predicted_dY += 28 + (countLines(desc) * 22) + 15;
                }
                for(auto& p : detailedEntity->passiveAbilities) { 
                    if (p == PassiveID::None) continue;
                    std::string desc = wrapText(getPassiveDescription(p), 42);
                    predicted_dY += 28 + (countLines(desc) * 22) + 15;
                }
                for(auto& a : detailedEntity->activeAbilities) { 
                    if (a == ActiveID::None) continue;
                    std::string desc = wrapText(getActiveDescription(a), 42);
                    predicted_dY += 28 + (countLines(desc) * 22) + 15;
                }
                
                int predicted_mY = 380;
                predicted_mY += detailedEntity->combatMenu.size() * 115;
                if (!detailedEntity->altCombatMenu.empty()) {
                    predicted_mY += 55;
                    predicted_mY += detailedEntity->altCombatMenu.size() * 115;
                }
                
                int maxAbsoluteY = std::max(predicted_dY, predicted_mY);
                int maxScroll = 0;
                
                if (maxAbsoluteY > 780) maxScroll = -(maxAbsoluteY - 760); 
                
                if (inspectScrollY < maxScroll) inspectScrollY = maxScroll;
                if (inspectScrollY > 0) inspectScrollY = 0;
                
                if (CheckCollisionPointRec(GetMousePosition(), {500, 300, 920, 480})) {
                    inspectScrollY += GetMouseWheelMove() * 40;
                    if (inspectScrollY < maxScroll) inspectScrollY = maxScroll;
                    if (inspectScrollY > 0) inspectScrollY = 0;
                }
                
                BeginScissorMode(500, 300, 920, 480);
                
                DrawText(TextFormat("INSPECTING: %s", detailedEntity->name.c_str()), 530, 320 + inspectScrollY, 40, GOLD);
                
                // --- COLUMN 1: Stats & Abilities (Left Side) ---
                DrawText(TextFormat("HP: %d/%d  |  ATK: %d  |  DEF: %d  |  SPD: %d", 
                     detailedEntity->currentHP, detailedEntity->maxHP,
                     detailedEntity->currentAttack, detailedEntity->currentDefense, 
                     detailedEntity->currentSpeed), 530, 380 + inspectScrollY, 22, GREEN);
                     
                DrawText(TextFormat("STM: %d/%d  |  BIQ: %d  |  SIQ: %d", 
                     detailedEntity->currentStamina, detailedEntity->maxStamina, 
                     detailedEntity->currentBIQ, detailedEntity->currentSIQ), 530, 420 + inspectScrollY, 22, GREEN);
                
                int dY = 490 + inspectScrollY; 
                DrawText("ABILITIES:", 530, dY, 25, SKYBLUE); dY += 40; 
                
                if (detailedEntity->naturalAbility != PassiveID::None) {
                    DrawText(TextFormat("[NATURAL] %s:", getPassiveName(detailedEntity->naturalAbility).c_str()), 530, dY, 20, GOLD); dY += 28;
                    std::string desc = wrapText(getPassiveDescription(detailedEntity->naturalAbility), 42);
                    DrawText(desc.c_str(), 550, dY, 18, LIGHTGRAY); 
                    dY += (countLines(desc) * 22) + 15;
                }
                
                for(auto& p : detailedEntity->passiveAbilities) { 
                    if (p == PassiveID::None) continue;
                    DrawText(TextFormat("[PASSIVE] %s:", getPassiveName(p).c_str()), 530, dY, 20, GREEN); dY += 28;
                    std::string desc = wrapText(getPassiveDescription(p), 42);
                    DrawText(desc.c_str(), 550, dY, 18, LIGHTGRAY); 
                    dY += (countLines(desc) * 22) + 15;
                }
                
                for(auto& a : detailedEntity->activeAbilities) { 
                    if (a == ActiveID::None) continue;
                    DrawText(TextFormat("[ACTIVE] %s:", getActiveName(a).c_str()), 530, dY, 20, ORANGE); dY += 28;
                    std::string desc = wrapText(getActiveDescription(a), 42);
                    DrawText(desc.c_str(), 550, dY, 18, LIGHTGRAY); 
                    dY += (countLines(desc) * 22) + 15;
                }

                // --- COLUMN 2: Moveset (Right Side) ---
                int mY = 380 + inspectScrollY;
                DrawText("MOVESET:", 1000, 340 + inspectScrollY, 25, PURPLE);
                for(auto& m : detailedEntity->combatMenu) {
                    DrawText(TextFormat("> %s [%d STM]", m.name.c_str(), m.staminaCost), 1000, mY, 22, WHITE); mY += 35;
                    DrawText(TextFormat("  Pow: %.1fx | Hits: %d | Tgt: %s", m.powerMultiplier, m.hitCount, getTargetText(m.target).c_str()), 1000, mY, 18, LIGHTGRAY); mY += 30;
                    DrawText(TextFormat("  Eff: %s", getEffectText(m.effect).c_str()), 1000, mY, 18, SKYBLUE); mY += 50;
                }
                
                // If they have an Alt Stance (Like Brian), print it right below their standard moves!
                if (!detailedEntity->altCombatMenu.empty()) {
                    mY += 15; // Extra spacing
                    DrawText("ALT STANCE MOVES:", 1000, mY, 25, PURPLE); mY += 40;
                    for(auto& m : detailedEntity->altCombatMenu) {
                        DrawText(TextFormat("> %s [%d STM]", m.name.c_str(), m.staminaCost), 1000, mY, 22, WHITE); mY += 35;
                        DrawText(TextFormat("  Pow: %.1fx | Hits: %d | Tgt: %s", m.powerMultiplier, m.hitCount, getTargetText(m.target).c_str()), 1000, mY, 18, LIGHTGRAY); mY += 30;
                        DrawText(TextFormat("  Eff: %s", getEffectText(m.effect).c_str()), 1000, mY, 18, SKYBLUE); mY += 50;
                    }
                }
                
                EndScissorMode(); // Stop clipping
            }
            else if (detailsType == 6) {
                std::string iName = ""; std::string iDesc = "";
                if (detailedItem == 1) { iName = "Bandage"; iDesc = "Restores 30% of Maximum HP."; }
                else if (detailedItem == 2) { iName = "Energy Drink"; iDesc = "Restores 40 Stamina instantly."; }
                else if (detailedItem == 3) { iName = "Revive"; iDesc = "Revives a knocked-out crew member with 50% HP."; }
                
                DrawText(iName.c_str(), 550, 350, 50, GOLD);
                DrawText("TYPE: CONSUMABLE", 550, 420, 30, LIGHTGRAY);
                DrawText("EFFECT:", 550, 490, 25, GREEN);
                DrawText(iDesc.c_str(), 550, 530, 25, WHITE);
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