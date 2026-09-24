#include "CombatUI.h" 
#include "CombatEngine.h" 
#include "AbilityDatabase.h"
#include "MoveDatabase.h" 
#include "../Systems/Logger.h" 
#include "../CORE/GlobalConstants.h"
#include "raylib.h" 
#include <string> 
#include <algorithm>
#include <map> // <--- Ensure this is here!

#include <map> 

extern std::map<ActorID, Texture2D> globalSprites;
// ---> ADD THIS NEW MAP <---
extern std::map<ActorID, Texture2D> globalAttackSprites; 

// --- ANIMATION TRACKERS ---
Entity* animAttacker = nullptr;
Vector2 animAttackerPos = {0,0};
Entity* animTarget = nullptr;
Vector2 animTargetOffset = {0,0};
Entity* animDying = nullptr;     
int deathFadeAlpha = 255;        

Vector2 GetBasePos(Entity* e, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam) {
    // Shrunk down to the native 320x180 canvas size
    Vector2 pPos[3] = { {33, 16}, {58, 40}, {33, 63} };
    Vector2 ePos[3] = { {266, 16}, {241, 40}, {266, 63} };
    
    for (int i = 0; i < pTeam.size(); i++) {
        if (pTeam[i] == e) return pPos[i];
    }
    for (int i = 0; i < eTeam.size(); i++) {
        if (eTeam[i] == e) return ePos[i];
    }
    return {0, 0};
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
    // --- 1. DRAW NATIVE WORLD (Background + Characters) ---
    Camera2D combatCam = { 0 };
    combatCam.zoom = 6.0f;
    
    BeginMode2D(combatCam);
    
    // (Your friend's 320x180 background texture will eventually be drawn right here!)

    float nativeBaseline = 25.0f; // Native floor height

    // Draw Player Sprites
    for (Entity* p : playerTeam) {
        if (!p->isAlive && p != animDying && p != animTarget) continue;
                 
        Color tint = WHITE; 
        if (p == animDying) tint = Fade(WHITE, deathFadeAlpha / 255.0f); 
                 
        Vector2 drawPos = GetBasePos(p, playerTeam, enemyTeam);
        if (p == animAttacker) drawPos = animAttackerPos;
        if (p == animTarget) { drawPos.x += animTargetOffset.x; drawPos.y += animTargetOffset.y; }

        if (globalSprites.count(p->actorID)) { 
            Texture2D activeTexture = globalSprites[p->actorID];
            if (p == animAttacker && globalAttackSprites.count(p->actorID)) {
                activeTexture = globalAttackSprites[p->actorID];
            }
            Rectangle fullSource = {0, 0, 32.0f, 32.0f};
            Rectangle destRect = { drawPos.x, drawPos.y + (nativeBaseline - 32.0f), 32.0f, 32.0f };
            DrawTexturePro(activeTexture, fullSource, destRect, {0,0}, 0.0f, tint);
        } else {
            DrawRectangle((int)drawPos.x, (int)(drawPos.y + (nativeBaseline - 8.0f)), 4, 8, BLUE); 
        }
    }
    
    // Draw Enemy Sprites
    for (Entity* e : enemyTeam) {
        if (!e->isAlive && e != animDying && e != animTarget) continue;
                 
        Color tint = WHITE; 
        if (e == animDying) tint = Fade(WHITE, deathFadeAlpha / 255.0f); 
                 
        Vector2 drawPos = GetBasePos(e, playerTeam, enemyTeam);
        if (e == animAttacker) drawPos = animAttackerPos;
        if (e == animTarget) { drawPos.x += animTargetOffset.x; drawPos.y += animTargetOffset.y; }

        if (globalSprites.count(e->actorID)) { 
            Texture2D activeTexture = globalSprites[e->actorID];
            if (e == animAttacker && globalAttackSprites.count(e->actorID)) {
                activeTexture = globalAttackSprites[e->actorID];
            }
            Rectangle fullSource = {0, 0, 32.0f, 32.0f};
            Rectangle destRect = { drawPos.x, drawPos.y + (nativeBaseline - 32.0f), 32.0f, 32.0f };
            DrawTexturePro(activeTexture, fullSource, destRect, {0,0}, 0.0f, tint);
        } else {
            DrawRectangle((int)drawPos.x, (int)(drawPos.y + (nativeBaseline - 8.0f)), 4, 8, RED); 
        }
    }
    EndMode2D(); // TURN OFF CAMERA

    // --- 2. DRAW 1080p UI ---
    DrawRectangle(0, 600, 1920, 480, DARKGRAY);
    DrawRectangleLines(0, 600, 1920, 480, WHITE);

    DrawText("MOMENTUM:", 700, 615, 25, YELLOW);
    DrawRectangle(860, 620, 200, 15, BLACK);
    DrawRectangle(860, 620, momentum * 2, 15, YELLOW);

    int uiY = 650;
    for (int i = 0; i < playerTeam.size(); i++) {
        Entity* p = playerTeam[i];
        if (!p) continue;
        
        Rectangle profileRect = { 50, (float)uiY, 80, 80 };
        if (globalSprites.count(p->actorID)) {
            Texture2D tex = globalSprites[p->actorID];
            // Lock the crop to the first 32x32 frame
            Rectangle sourceCrop = {0, 0, 32.0f, 32.0f}; 
            DrawTexturePro(tex, sourceCrop, profileRect, {0,0}, 0.0f, p->isAlive ? WHITE : DARKGRAY);
            DrawRectangleLinesEx(profileRect, 2, WHITE);
        } else {
            DrawRectangleRec(profileRect, p->isAlive ? BLUE : DARKGRAY); DrawRectangleLinesEx(profileRect, 2, WHITE);
            DrawText(p->name.substr(0, 3).c_str(), 65, uiY + 30, 20, WHITE); 
        }
        
        if (!p->isAlive) { DrawText("KNOCKED OUT", 150, uiY + 30, 20, RED); uiY += 120; continue; }

        DrawText(TextFormat("%s [Lv %d]", p->name.c_str(), p->level), 150, uiY, 20, WHITE);

        int displayHP = p->currentHP + p->shieldHP;
        DrawText(TextFormat("HP: %d / %d", displayHP, p->maxHP), 150, uiY + 25, 18, GREEN);
        DrawRectangle(150, uiY + 45, 200, 15, BLACK);
        float hpPercent = (float)p->currentHP / p->maxHP; if (hpPercent > 1.0f) hpPercent = 1.0f;
        DrawRectangle(150, uiY + 45, hpPercent * 200, 15, GREEN);
        
        if (p->shieldHP > 0) {
            float shieldPercent = (float)p->shieldHP / p->maxHP;
            if (hpPercent + shieldPercent > 1.0f) shieldPercent = 1.0f - hpPercent;
            DrawRectangle(150 + (hpPercent * 200), uiY + 45, shieldPercent * 200, 15, SKYBLUE);
        }

        DrawText(TextFormat("STM: %d / %d", p->currentStamina, p->maxStamina), 150, uiY + 65, 18, YELLOW);
        DrawRectangle(150, uiY + 85, 200, 15, BLACK);
        float stamPercent = (float)p->currentStamina / p->maxStamina;
        DrawRectangle(150, uiY + 85, stamPercent * 200, 15, YELLOW);

        int statX = 370; int statY = uiY + 20;
        std::map<std::string, int> statusCounts; std::map<std::string, Color> statusColors; std::vector<std::string> displayOrder; 

        for (auto& s : p->activeStatuses) {
            std::string label = ""; Color statCol = WHITE;
            if (s.category == StatusCategory::Buff || s.category == StatusCategory::Debuff) {
                std::string statStr = (s.statModifier > 0) ? "UP" : "DOWN"; statCol = (s.statModifier > 0) ? GREEN : RED;
                if (s.targetStat == StatName::ATK) label = "ATK " + statStr; else if (s.targetStat == StatName::DEF) label = "DEF " + statStr;
                else if (s.targetStat == StatName::SPD) label = "SPD " + statStr; else if (s.targetStat == StatName::BIQ) label = "BIQ " + statStr; else if (s.targetStat == StatName::SIQ) label = "SIQ " + statStr;
            } else { label = s.name; statCol = RED; }

            if (label != "") {
                if (statusCounts[label] == 0) { displayOrder.push_back(label); statusColors[label] = statCol; }
                statusCounts[label]++;
            }
        }
        for (const std::string& label : displayOrder) {
            std::string finalText = label; if (statusCounts[label] > 1) finalText += " x" + std::to_string(statusCounts[label]);
            DrawText(finalText.c_str(), statX, statY, 15, statusColors[label]); statY += 20;
            if (statY > uiY + 80) { statY = uiY + 20; statX += 100; } 
        }
        uiY += 120;
    }

    // COMBAT LOG
    static int combatLogScrollY = 0; static int lastLogCount = 0; 
    Rectangle logRec = { 1200, 600, 720, 480 }; 
    DrawRectangleRec(logRec, Fade(BLACK, 0.5f)); DrawRectangleLinesEx(logRec, 2, WHITE); DrawText("COMBAT LOG", 1220, 620, 25, LIGHTGRAY);
         
    std::vector<std::string> logs = GameLog::GetMessages(); 
    int totalLogHeight = logs.size() * 30; 
    int maxScroll = 0; if (totalLogHeight > 410) maxScroll = -(totalLogHeight - 410); 
         
    int currentTotalLogs = GameLog::GetTotalMessagesLogged();
    if (currentTotalLogs < lastLogCount) lastLogCount = currentTotalLogs;
    if (currentTotalLogs > lastLogCount) { combatLogScrollY = maxScroll; lastLogCount = currentTotalLogs; }
         
    if (combatLogScrollY < maxScroll) combatLogScrollY = maxScroll;
    if (combatLogScrollY > 0) combatLogScrollY = 0;
    if (CheckCollisionPointRec(GetMousePosition(), logRec)) {
        combatLogScrollY += GetMouseWheelMove() * 30;
        if (combatLogScrollY > 0) combatLogScrollY = 0; if (combatLogScrollY < maxScroll) combatLogScrollY = maxScroll;
    }
    
    BeginScissorMode(1200, 660, 720, 410);
    int logY = 670 + combatLogScrollY;
    for (std::string& msg : logs) { DrawText(msg.c_str(), 1220, logY, 20, WHITE); logY += 30; }
    EndScissorMode();

    // 3. DRAW DYNAMIC OVERLAYS IN 1080P SPACE
    
    // --- ADD THIS BLOCK FOR PLAYER NAMES ---
    for (Entity* p : playerTeam) {
        if (!p->isAlive && p != animDying && p != animTarget) continue;
        
        Vector2 drawPos = GetBasePos(p, playerTeam, enemyTeam);
        if (p == animAttacker) drawPos = animAttackerPos;
        if (p == animTarget) { drawPos.x += animTargetOffset.x; drawPos.y += animTargetOffset.y; }
        
        // Translates native position to 1080p screen placement!
        float screenX = drawPos.x * 6.0f;
        float screenY = drawPos.y * 6.0f;
        
        Color textColor = WHITE;
        if (p == animDying) textColor = Fade(textColor, deathFadeAlpha / 255.0f);
        
        DrawText(p->name.c_str(), screenX - 10, screenY - 30, 25, textColor);
    }

    // 3. DRAW ENEMY OVERLAYS IN 1080P SPACE
    for (Entity* e : enemyTeam) {
        if (!e->isAlive && e != animDying && e != animTarget) continue;
        
        Vector2 drawPos = GetBasePos(e, playerTeam, enemyTeam);
        if (e == animAttacker) drawPos = animAttackerPos;
        if (e == animTarget) { drawPos.x += animTargetOffset.x; drawPos.y += animTargetOffset.y; }
        
        // Translates native position to 1080p screen placement!
        float screenX = drawPos.x * 6.0f;
        float screenY = drawPos.y * 6.0f;
        
        Color hpColor = RED; Color textColor = WHITE; Color barBgColor = BLACK;
        if (e == animDying) { 
            float a = deathFadeAlpha / 255.0f;
            hpColor = Fade(hpColor, a); textColor = Fade(textColor, a); barBgColor = Fade(barBgColor, a);
        }
        
        DrawText(e->name.c_str(), screenX - 10, screenY - 30, 25, textColor);
        
        if (e->shieldHP > 0) {
            int shieldWidth = (e->shieldHP * 140) / e->maxHP;
            if (shieldWidth > 140) shieldWidth = 140; 
            DrawRectangle(screenX - 35, screenY + 155, 140, 10, barBgColor);
            DrawRectangle(screenX - 35, screenY + 155, shieldWidth, 10, SKYBLUE);
        }
        DrawRectangle(screenX - 35, screenY + 165, 140, 15, barBgColor); 
        DrawRectangle(screenX - 35, screenY + 165, (e->currentHP * 140) / e->maxHP, 15, hpColor);
        DrawText(TextFormat("%d / %d", e->currentHP, e->maxHP), screenX - 35, screenY + 185, 20, textColor);
    }
}

void AnimateApproach(Entity* attacker, Entity* target, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum) {
    Vector2 start;
    if (animAttacker == attacker) { start = animAttackerPos; } 
    else { start = GetBasePos(attacker, pTeam, eTeam); }
    
    Vector2 end = GetBasePos(target, pTeam, eTeam);
    
    // --- THE FIX: Shrunk the 1080p distance (120) down to native space (20) ---
    if (start.x < end.x) end.x -= 20; else end.x += 20; 
    
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
    
    // --- THE FIX: Track dynamically if the character is currently moving! ---
    Vector2 activePos = GetBasePos(target, pTeam, eTeam);
    if (target == animAttacker) activePos = animAttackerPos;

    for (int i=0; i<durationFrames; i++) { 
        if (!isDodge && damage >= 0 && i < 15) { 
            // Shrink the native shake so they don't vibrate wildly out of bounds
            animTargetOffset.x = (GetRandomValue(0, 20) - 10) / 6.0f; 
            animTargetOffset.y = (GetRandomValue(0, 20) - 10) / 6.0f;
        } else {
            animTargetOffset = {0,0}; 
        }
        
        // Re-calculate screen position every frame because the target might be shaking!
        float screenX = (activePos.x + animTargetOffset.x) * 6.0f;
        float screenY = (activePos.y + animTargetOffset.y) * 6.0f;

        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(pTeam, eTeam, momentum);
        int slideUp = (i < 20) ? (i*2) : 40;
        
        DrawText(popupText.c_str(), screenX + 10, screenY - 50 - slideUp, isCrit? 40 : 30, popColor);
        EndDrawing();
    }
    animTargetOffset = {0,0}; animTarget = nullptr;
}

void AnimatePopupsForEntity(Entity* target, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum) {
    if (target->pendingPopups.empty()) return;

    // --- THE FIX: Track dynamically if the character is currently moving! ---
    Vector2 activePos = GetBasePos(target, pTeam, eTeam);
    if (target == animAttacker) activePos = animAttackerPos;
    if (target == animTarget) { activePos.x += animTargetOffset.x; activePos.y += animTargetOffset.y; }

    // Translate Native Space to 1080p Screen Space
    float screenX = activePos.x * 6.0f;
    float screenY = activePos.y * 6.0f;

    for (auto& popup : target->pendingPopups) {
        Color pCol = popup.isBuff ? GREEN : RED; 

        for (int i=0; i<30; i++) {
            BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(pTeam, eTeam, momentum);

            int slideUp = i; 
            float alpha = 1.0f;
            if (i > 40) alpha = 1.0f - ((i - 40) / 20.0f); 

            // Safely anchor the text directly above their head
            DrawText(popup.text.c_str(), screenX - 10, screenY - 40 - slideUp, 20, Fade(pCol, alpha));

            EndDrawing();
        }
    }
    target->pendingPopups.clear();
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
        // --- THE FIX: Shrunk the jump peak down to 4 pixels natively ---
        animAttackerPos.y -= (i < 10) ? (i * 0.4f) : ((20 - i) * 0.4f); 
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(pTeam, eTeam, momentum); EndDrawing();
    }
    animAttacker = nullptr;
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
        
        DrawRectangle(550, 650, 630, 400, Fade(DARKPURPLE, 0.6f)); 
        DrawRectangleLines(550, 650, 630, 400, PURPLE);
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
    GameLog::Add(">>> " + character->name + " is stunned and loses their turn! <<<"); character->removeStatusByType(StatusCategory::HardCC); pauseForPlayer(false, playerTeam, enemyTeam, momentum);
}

std::vector<Entity*> requestPlayerTargets(Entity* attacker, const Move& selectedMove, std::vector<Entity*>& enemyTeam, std::vector<Entity*>& playerTeam) {
    std::vector<Entity*> selectedTargets;
    
    // <--- NEW: Reserve enough space for whichever team is larger
    selectedTargets.reserve(std::max(playerTeam.size(), enemyTeam.size())); 
    
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
        
        DrawRectangle(550, 650, 630, 400, Fade(DARKPURPLE, 0.6f)); 
        DrawRectangleLines(550, 650, 630, 400, PURPLE);
        
        DrawText(TextFormat("SELECT TARGET(S) FOR: %s (%d needed)", selectedMove.name.c_str(), targetsNeeded - selectedTargets.size()), 580, 670, 25, WHITE);
        DrawText("[ HOVER OVER A TARGET AND CLICK, OR PRESS 'B' TO CANCEL ]", 580, 710, 18, LIGHTGRAY);
        
        // --- TRANSLATE MOUSE TO WORLD SPACE ---
        Camera2D combatCam = { 0 }; combatCam.zoom = 6.0f;
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), combatCam);

        for (Entity* e : validPool) {
            if (e->isAlive) {
                Vector2 pos = GetBasePos(e, playerTeam, enemyTeam);
                
                float hitboxW = 6.0f; 
                float hitboxH = 8.0f; 
                
                if (globalSprites.count(e->actorID)) { 
                    hitboxW = 32.0f;
                    hitboxH = 32.0f;
                }
                
                // Matches the native baseline height!
                Rectangle targetBox = { pos.x, pos.y + (25.0f - hitboxH), hitboxW, hitboxH };
                
                // --- THE FIX: Translate native hitbox to 1080p screen space for drawing! ---
                Rectangle screenBox = { targetBox.x * 6.0f, targetBox.y * 6.0f, targetBox.width * 6.0f, targetBox.height * 6.0f };
                
                bool alreadySelected = (std::find(selectedTargets.begin(), selectedTargets.end(), e) != selectedTargets.end());
                if (alreadySelected) { 
                    DrawRectangleLinesEx(screenBox, 4, GREEN);
                    DrawText("SELECTED", screenBox.x - 15, screenBox.y - 30, 20, GREEN); 
                } else {
                    if (CheckCollisionPointRec(mouseWorldPos, targetBox)) {
                        DrawRectangleLinesEx(screenBox, 4, YELLOW);
                        DrawText("TARGET", screenBox.x - 5, screenBox.y - 30, 20, YELLOW);
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

void executePlayerTurn(Entity* character, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, std::vector<Entity*>& masterRoster, std::vector<TeamUpSkill>& masterTeamUps, int* inventory, int& wallet, int& teamMomentum, std::vector<TeamUpSkill>& previouslyActiveTeamUps, bool& battleIsActive, bool canFlee) {
    
    static bool showDetailsPopup = false;
    
    // --- THE FIX: DECLARE THE POPUP VARIABLES ---
    static std::string detailsPopupTitle = "";
    static std::string detailsPopupText = "";
    // --------------------------------------------
    bool turnComplete = false; bool inItemMenu = false; bool inReviveMenu = false; bool inTeamUpMenu = false; bool inSwapMenu = false; bool inActiveMenu = false;
    ActiveID detailedActive = ActiveID::None; // For the right-click menu
    int swapGroup = -1; int swapIndex = -1; int detailedItem = 0; // For right-clicking backpack items
         
    Move detailedMove; Entity* detailedEntity = nullptr; int detailsType = 0; int pendingMomentumCost = 0;
    
    // --- NEW: Add this variable to track which Team Up we right-clicked ---
    std::string detailedTeamUpName = "";
    
    while (!turnComplete && battleIsActive && !WindowShouldClose()) {
        Move selectedMove; bool moveSelected = false;
        BeginDrawing(); ClearBackground(DARKBLUE); DrawBattleOverlay(playerTeam, enemyTeam, teamMomentum);
        
        // NARROWED BY 70px TO MAKE ROOM FOR COMBAT LOG
        DrawRectangle(550, 650, 630, 400, Fade(DARKBLUE, 0.5f)); 
        DrawRectangleLines(550, 650, 630, 400, WHITE);
        
        // 1. Update this line so the main menu hides when you open the Active Skills menu!
        bool isMainMenu = (!inItemMenu && !inReviveMenu && !inTeamUpMenu && !inSwapMenu && !inActiveMenu);
        
        // Setup mouse translation for sprites
        Camera2D combatCam = { 0 }; combatCam.zoom = 6.0f;
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), combatCam);
        Vector2 mouseScreenPos = GetMousePosition();

        if (!showDetailsPopup) {
            // Check Enemy Sprite Clicks
            for (Entity* e : enemyTeam) {
                if (!e->isAlive) continue; 
                Vector2 pos = GetBasePos(e, playerTeam, enemyTeam);
                
                float hitboxW = 6.0f; float hitboxH = 8.0f;
                if (globalSprites.count(e->actorID)) {
                    hitboxW = 32.0f; hitboxH = 32.0f;
                }
                Rectangle targetBox = { pos.x, pos.y + (25.0f - hitboxH), hitboxW, hitboxH };

                if (CheckCollisionPointRec(mouseWorldPos, targetBox) && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) { detailedEntity = e; detailsType = 5; showDetailsPopup = true; }
            }
            
            // Check Player Sprite AND Profile Clicks
            for (int i = 0; i < playerTeam.size(); i++) {
                Entity* p = playerTeam[i];
                if (!p->isAlive) continue; 
                Vector2 pos = GetBasePos(p, playerTeam, enemyTeam);
                
                float hitboxW = 6.0f; float hitboxH = 8.0f;
                if (globalSprites.count(p->actorID)) {
                    hitboxW = 32.0f; hitboxH = 32.0f;
                }
                Rectangle targetBox = { pos.x, pos.y + (25.0f - hitboxH), hitboxW, hitboxH };

                // Hitbox for Sprite (Native World Space)
                if (CheckCollisionPointRec(mouseWorldPos, targetBox) && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) { detailedEntity = p; detailsType = 5; showDetailsPopup = true; }
                
                // Hitbox for Profile Picture (1080p Screen Space)
                if (CheckCollisionPointRec(mouseScreenPos, {50, 650.0f + (i * 120), 80, 80}) && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) { detailedEntity = p; detailsType = 5; showDetailsPopup = true; }
            }
        }
        
        // 3. Use the SINGLE unified scrolling block here
        if (isMainMenu) {
            DrawText(TextFormat("WHAT WILL %s DO?", character->name.c_str()), 580, 670, 30, WHITE);
            DrawText("[ Scroll to see more options | Right-Click to Inspect! ]", 580, 710, 18, LIGHTGRAY);
            
            // --- NEW: DYNAMIC STANCE TRACKERS ---
            if (character->naturalAbility == NaturalID::ScrewDat) {
                if (character->isAltStance) {
                    // Calculates exact current charge (Max 120%)
                    int currentBoost = std::min(120, character->screwDatStacks * 30);
                    DrawText(TextFormat("ULT CHARGE: +%d%%", currentBoost), 950, 675, 20, ORANGE);
                } else {
                    // Shows the permanent stacking heal power
                    DrawText(TextFormat("HEAL POWER: %d%%", character->supportHealStacks), 950, 675, 20, GREEN);
                }
            }
            // ------------------------------------
            
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

            // ---> THE NATURAL ACTIVE BUTTON <---
            if (character->naturalAbility == NaturalID::Mediator) {
                bool canUseNat = canUseNaturalActive(*character, playerTeam);
                Rectangle medBtnRec = {900, (float)rightY, 250, 40};
                
                if (DrawGUIButton(medBtnRec, TextFormat("%d. Mediator", nextBtnNum), (nextBtnNum == 8 ? KEY_EIGHT : KEY_NINE), showDetailsPopup || !canUseNat)) {
                    if (canUseNat) {
                        executeNaturalActive(*character, playerTeam, enemyTeam);
                        turnComplete = true; 
                    }
                }
                
                // --- THE FIX: ADD DETAILS TYPE 3 ---
                if (CheckCollisionPointRec(GetMousePosition(), medBtnRec) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !showDetailsPopup) {
                    detailsPopupTitle = "Natural Ability: Mediator";
                    detailsPopupText = "Take the heat. Heal 50% HP, gain Taunt & Stun Immunity.\nPassive: -15% DMG Taken. Awakens (-30% total DMG Taken) if an ally is <20% HP or faints.";
                    detailsType = 3; // <--- THIS WAS MISSING!
                    showDetailsPopup = true; 
                }
                
                rightY += 50;
                nextBtnNum++;
            }
            
            if (!character->activeAbilities.empty()) {
                if (DrawGUIButton({900, (float)rightY, 250, 40}, TextFormat("%d. Active Skill", nextBtnNum), (nextBtnNum == 8 ? KEY_EIGHT : KEY_NINE), showDetailsPopup)) {
                    inActiveMenu = true;
                }
                rightY += 50;
                nextBtnNum++;
            }
            
            if (!character->altCombatMenu.empty()) {
                if (DrawGUIButton({900, (float)rightY, 250, 40}, TextFormat("%d. Stance (%s)", nextBtnNum, character->isAltStance ? "Support" : "Strike"), (nextBtnNum == 8 ? KEY_EIGHT : KEY_NINE), showDetailsPopup)) { 
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
                    if (DrawGUIButton({ 580.0f, (float)startY + (btnIndex * 50), 300.0f, 40.0f }, (std::to_string(btnIndex + 1) + ". " + p->name).c_str(), buttonKeys[btnIndex], showDetailsPopup)) { 
                        inventory[2]--; 
                        p->isAlive = true; 
                        
                        // ---> THE FIX 1: Half HP, Half Stamina, and clear all buffs/debuffs <---
                        p->currentHP = p->maxHP / 2; 
                        p->currentStamina = p->maxStamina / 2;
                        p->resetStats(); 
                        // -----------------------------------------------------------------------
                        
                        GameLog::Add(character->name + " used a Revive on " + p->name + "!"); 
                        turnComplete = true; 
                        break; 
                    }
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
            
            // Shifted Cancel button left to 950 so it fits safely within the 1180px right-edge boundary
            if (DrawGUIButton({950, 750, 200, 40}, "0. Cancel [B]", KEY_ZERO, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) inTeamUpMenu = false;
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
                         // ---> THE FIX: Grab the exact pointers BEFORE we overwrite their slots! <---
                         Entity* swappedA = getEntity(swapGroup, swapIndex);
                         Entity* swappedB = getEntity(g, i);
                                                 
                         if (swapGroup == 0) tempActive[swapIndex] = swappedB;
                         else tempReserves[swapIndex] = swappedB;
                                                 
                         if (g == 0) tempActive[i] = swappedA;
                         else tempReserves[i] = swappedA;
                                                 
                         playerTeam.clear();
                         for (auto* ent : tempActive) if (ent != nullptr) playerTeam.push_back(ent);
                                                 
                         masterRoster.clear();
                         for (auto* ent : tempReserves) if (ent != nullptr) masterRoster.push_back(ent);
                         
                         // ---> NEW: Log the switch safely to the Combat Log! <---
                         std::string nameA = swappedA ? swappedA->name : "Empty Slot";
                         std::string nameB = swappedB ? swappedB->name : "Empty Slot";
                         GameLog::Add(">>> " + nameA + " swapped places with " + nameB + "! <<<");
                                                 
                         // If the character currently taking their turn was swapped, hand the turn to the new character!
                         if (character == swappedA && swappedB != nullptr) character = swappedB;
                         else if (character == swappedB && swappedA != nullptr) character = swappedA;

                         swapGroup = -1; swapIndex = -1;
                         
                         // Automatically close the swap menu to speed up combat!
                         inSwapMenu = false; 
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

            // --- SQUISHED COLUMNS TO FIT NEW UI BOUNDS ---
            DrawText("ACTIVE:", 570, 765, 20, GREEN);
            for (int i=0; i < 3; i++) DrawSlot({570, 790.0f + (i * 45), 180, 40}, 0, i);
            
            DrawText("RESERVES:", 770, 765, 20, GOLD);
            for (int i=0; i < 3; i++) DrawSlot({770, 790.0f + (i * 45), 180, 40}, 1, i);
            
            if (DrawGUIButton({970, 790, 180, 40}, "0. Back [B]", KEY_ZERO, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) {
                inSwapMenu = false; swapGroup = -1; swapIndex = -1;
            }
        }

        if (showDetailsPopup) {
            DrawRectangle(0, 0, 1920, 1080, Fade(BLACK, 0.95f));
            
            // --- NEW: MASSIVELY EXPANDED BOX ---
            DrawRectangle(360, 200, 1200, 680, DARKGRAY); 
            DrawRectangleLinesEx({360, 200, 1200, 680}, 4, WHITE);
            
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
            else if (detailsType == 2) {
                for (TeamUpSkill& tu : masterTeamUps) {
                    if (tu.name == detailedTeamUpName) {
                        DrawText(tu.name.c_str(), 550, 350, 50, GOLD); 
                        DrawText(TextFormat("MOMENTUM COST: %d%%", tu.momentumCost), 550, 420, 30, YELLOW);
                        DrawText(TextFormat("HITS: %d", tu.moveData.hitCount), 550, 470, 30, WHITE); 
                        DrawText(TextFormat("POWER MULTIPLIER: %.1fx", tu.moveData.powerMultiplier), 550, 520, 30, ORANGE);
                        DrawText(TextFormat("TARGETS: %s", getTargetText(tu.moveData.target).c_str()), 550, 590, 30, SKYBLUE); 
                        DrawText(TextFormat("EFFECT: %s", getEffectText(tu.moveData.effect).c_str()), 550, 640, 30, PURPLE);
                        
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
            else if (detailsType == 3) {
                // --- NEW: THE MEDIATOR DRAWING BLOCK ---
                DrawText(detailsPopupTitle.c_str(), 550, 350, 50, GOLD);
                DrawText("TYPE: NATURAL ABILITY", 550, 420, 30, LIGHTGRAY);
                DrawText("EFFECT:", 550, 490, 25, GREEN);
                
                // We use a slightly smaller font (22) for the description so the line breaks fit nicely!
                DrawText(detailsPopupText.c_str(), 550, 530, 22, WHITE);
            }
            else if (detailsType == 4) {
                DrawText(getActiveName(detailedActive).c_str(), 550, 350, 50, GOLD);
                DrawText("TYPE: ACTIVE", 550, 420, 30, LIGHTGRAY);
                DrawText("EFFECT:", 550, 490, 25, ORANGE);
                DrawText(getActiveDescription(detailedActive).c_str(), 550, 530, 25, WHITE);
            }
            else if (detailsType == 5 && detailedEntity != nullptr) {
                static int inspectScrollY = 0;
                
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
                
                auto countLines = [](const std::string& text) {
                    int lines = 1;
                    for(char c : text) if(c == '\n') lines++;
                    return lines;
                };
                
                int predicted_dY = 410; 
                if (detailedEntity->naturalAbility != NaturalID::None) {
                    std::string desc = wrapText(getNaturalDescription(detailedEntity->naturalAbility), 55);
                    predicted_dY += 28 + (countLines(desc) * 22) + 15;
                }
                for(auto& p : detailedEntity->passiveAbilities) { 
                    if (p == PassiveID::None) continue;
                    std::string desc = wrapText(getPassiveDescription(p), 55);
                    predicted_dY += 28 + (countLines(desc) * 22) + 15;
                }
                for(auto& a : detailedEntity->activeAbilities) { 
                    if (a == ActiveID::None) continue;
                    std::string desc = wrapText(getActiveDescription(a), 55);
                    predicted_dY += 28 + (countLines(desc) * 22) + 15;
                }

                // ---> NEW: AGGREGATE STATS TO EXPAND SCROLL AREA <---
                int netATK = 0, netDEF = 0, netSPD = 0, netBIQ = 0, netSIQ = 0;
                bool hasStun = false, hasTaunt = false, hasStamDown = false;
                int bleedDamage = 0;

                for (auto& s : detailedEntity->activeStatuses) {
                    if (s.targetStat == StatName::ATK) netATK += s.statModifier;
                    if (s.targetStat == StatName::DEF) netDEF += s.statModifier;
                    if (s.targetStat == StatName::SPD) netSPD += s.statModifier;
                    if (s.targetStat == StatName::BIQ) netBIQ += s.statModifier;
                    if (s.targetStat == StatName::SIQ) netSIQ += s.statModifier;
                    
                    if (s.causesStun || s.category == StatusCategory::HardCC) hasStun = true;
                    if (s.causesTaunt) hasTaunt = true;
                    if (s.halvesStaminaRegen) hasStamDown = true;
                    if (s.dotDamage > 0) bleedDamage += s.dotDamage;
                }

                int effectCount = 0;
                if (netATK != 0) effectCount++;
                if (netDEF != 0) effectCount++;
                if (netSPD != 0) effectCount++;
                if (netBIQ != 0) effectCount++;
                if (netSIQ != 0) effectCount++;
                if (detailedEntity->hitNullificationStacks > 0) effectCount++;
                if (detailedEntity->shieldHP > 0) effectCount++;
                if (hasStun) effectCount++;
                if (hasTaunt) effectCount++;
                if (hasStamDown) effectCount++;
                if (bleedDamage > 0) effectCount++;
                if (effectCount == 0) effectCount = 1; // Reserves space for "[ NONE ]"

                predicted_dY += 45 + (effectCount * 25);
                // ----------------------------------------------------

                int predicted_mY = 280;
                predicted_mY += detailedEntity->combatMenu.size() * 115;
                if (!detailedEntity->altCombatMenu.empty()) {
                    predicted_mY += 55;
                    predicted_mY += detailedEntity->altCombatMenu.size() * 115;
                }
                
                int maxAbsoluteY = std::max(predicted_dY, predicted_mY);
                int maxScroll = 0;
                
                if (maxAbsoluteY > 860) maxScroll = -(maxAbsoluteY - 840); 
                
                if (inspectScrollY < maxScroll) inspectScrollY = maxScroll;
                if (inspectScrollY > 0) inspectScrollY = 0;
                
                if (CheckCollisionPointRec(GetMousePosition(), {360, 200, 1200, 680})) {
                    inspectScrollY += GetMouseWheelMove() * 40;
                    if (inspectScrollY < maxScroll) inspectScrollY = maxScroll;
                    if (inspectScrollY > 0) inspectScrollY = 0;
                }
                
                BeginScissorMode(360, 200, 1200, 680);
                
                DrawText(TextFormat("INSPECTING: %s", detailedEntity->name.c_str()), 400, 230 + inspectScrollY, 40, GOLD);
                
                DrawText(TextFormat("HP: %d/%d  |  ATK: %d  |  DEF: %d  |  SPD: %d", 
                      detailedEntity->currentHP, detailedEntity->maxHP, 
                    detailedEntity->currentAttack, detailedEntity->currentDefense, 
                      detailedEntity->currentSpeed), 400, 290 + inspectScrollY, 22, GREEN);
                      
                DrawText(TextFormat("STM: %d/%d  |  BIQ: %d  |  SIQ: %d", 
                      detailedEntity->currentStamina, detailedEntity->maxStamina, 
                      detailedEntity->currentBIQ, detailedEntity->currentSIQ), 400, 330 + inspectScrollY, 22, GREEN);
                
                int dY = 390 + inspectScrollY; 
                DrawText("ABILITIES:", 400, dY, 25, SKYBLUE); dY += 40; 
                
                if (detailedEntity->naturalAbility != NaturalID::None) {
                    DrawText(TextFormat("[NATURAL] %s:", getNaturalName(detailedEntity->naturalAbility).c_str()), 400, dY, 20, GOLD); dY += 28;
                    std::string desc = wrapText(getNaturalDescription(detailedEntity->naturalAbility), 55); 
                    DrawText(desc.c_str(), 420, dY, 18, LIGHTGRAY); 
                    dY += (countLines(desc) * 22) + 15;
                }
                
                for(auto& p : detailedEntity->passiveAbilities) { 
                    if (p == PassiveID::None) continue;
                    DrawText(TextFormat("[PASSIVE] %s:", getPassiveName(p).c_str()), 400, dY, 20, GREEN); dY += 28;
                    std::string desc = wrapText(getPassiveDescription(p), 55);
                    DrawText(desc.c_str(), 420, dY, 18, LIGHTGRAY); 
                    dY += (countLines(desc) * 22) + 15;
                }
                
                for(auto& a : detailedEntity->activeAbilities) { 
                    if (a == ActiveID::None) continue;
                    DrawText(TextFormat("[ACTIVE] %s:", getActiveName(a).c_str()), 400, dY, 20, ORANGE); dY += 28;
                    std::string desc = wrapText(getActiveDescription(a), 55);
                    DrawText(desc.c_str(), 420, dY, 18, LIGHTGRAY); 
                    dY += (countLines(desc) * 22) + 15;
                }

                // ---> NEW: DRAW THE NET STATUS EFFECTS BOARD <---
                dY += 15;
                DrawText("ACTIVE STATUS EFFECTS:", 400, dY, 25, RED); dY += 35;

                if (effectCount == 1 && netATK == 0 && netDEF == 0 && netSPD == 0 && netBIQ == 0 && netSIQ == 0 && detailedEntity->hitNullificationStacks == 0 && detailedEntity->shieldHP == 0 && !hasStun && !hasTaunt && !hasStamDown && bleedDamage == 0) {
                    DrawText("[ NONE ]", 420, dY, 20, DARKGRAY); dY += 25;
                } else {
                    // Lambda helper to instantly format and draw stat lines
                    auto PrintStatEffect = [&](const char* name, int netVal, int baseVal) {
                        if (netVal == 0) return;
                        int pct = (baseVal > 0) ? (int)(((float)netVal / baseVal) * 100.0f) : 0;
                        Color col = (netVal > 0) ? GREEN : RED;
                        std::string sign = (netVal > 0) ? "+" : "";
                        std::string text = TextFormat("> %s: %s%d%% (%s%d Flat)", name, sign.c_str(), pct, sign.c_str(), netVal);
                        DrawText(text.c_str(), 420, dY, 20, col);
                        dY += 25;
                    };
                    
                    PrintStatEffect("Attack", netATK, detailedEntity->baseAttack);
                    PrintStatEffect("Defense", netDEF, detailedEntity->baseDefense);
                    PrintStatEffect("Speed", netSPD, detailedEntity->baseSpeed);
                    PrintStatEffect("Battle IQ", netBIQ, detailedEntity->baseBIQ);
                    PrintStatEffect("Strategic IQ", netSIQ, detailedEntity->baseSIQ);
                    
                    if (detailedEntity->hitNullificationStacks > 0) {
                        DrawText(TextFormat("> Hit Nullification: x%d", detailedEntity->hitNullificationStacks), 420, dY, 20, SKYBLUE); dY += 25;
                    }
                    if (detailedEntity->shieldHP > 0) {
                        DrawText(TextFormat("> Shield: %d HP", detailedEntity->shieldHP), 420, dY, 20, SKYBLUE); dY += 25;
                    }
                    if (hasStun) {
                        DrawText("> Stunned: Cannot act this turn.", 420, dY, 20, RED); dY += 25;
                    }
                    if (hasTaunt) {
                        DrawText("> Taunting: Drawing all enemy attacks.", 420, dY, 20, ORANGE); dY += 25;
                    }
                    if (hasStamDown) {
                        DrawText("> Wind Knocked Out: Stamina Regen Halved.", 420, dY, 20, RED); dY += 25;
                    }
                    if (bleedDamage > 0) {
                        DrawText(TextFormat("> Bleeding: Takes %d DMG at end of turn.", bleedDamage), 420, dY, 20, RED); dY += 25;
                    }
                }
                // ----------------------------------------------------

                // --- COLUMN 2: Pushed to the right ---
                int mY = 280 + inspectScrollY;
                DrawText("MOVESET:", 1020, 240 + inspectScrollY, 25, PURPLE);
                for(auto& m : detailedEntity->combatMenu) {
                    DrawText(TextFormat("> %s [%d STM]", m.name.c_str(), m.staminaCost), 1020, mY, 22, WHITE); mY += 35;
                    DrawText(TextFormat("  Pow: %.1fx | Hits: %d | Tgt: %s", m.powerMultiplier, m.hitCount, getTargetText(m.target).c_str()), 1020, mY, 18, LIGHTGRAY); mY += 30;
                    DrawText(TextFormat("  Eff: %s", getEffectText(m.effect).c_str()), 1020, mY, 18, SKYBLUE); mY += 50;
                }
                
                if (!detailedEntity->altCombatMenu.empty()) {
                    mY += 15; 
                    DrawText("ALT STANCE MOVES:", 1020, mY, 25, PURPLE); mY += 40;
                    for(auto& m : detailedEntity->altCombatMenu) {
                        DrawText(TextFormat("> %s [%d STM]", m.name.c_str(), m.staminaCost), 1020, mY, 22, WHITE); mY += 35;
                        DrawText(TextFormat("  Pow: %.1fx | Hits: %d | Tgt: %s", m.powerMultiplier, m.hitCount, getTargetText(m.target).c_str()), 1020, mY, 18, LIGHTGRAY); mY += 30;
                        DrawText(TextFormat("  Eff: %s", getEffectText(m.effect).c_str()), 1020, mY, 18, SKYBLUE); mY += 50;
                    }
                }
                EndScissorMode();
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
            
            // --- MOVED THE BUTTON SAFELY BELOW THE BOX ---
            if (DrawGUIButton({810, 900, 300, 60}, "Close Info [B]", 0) || IsKeyReleased(KEY_B)) showDetailsPopup = false;
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
    bool allPlayersDead = true; for (Entity* player : playerTeam) if (player->isAlive) allPlayersDead = false; for (Entity* reserve : masterRoster) {
    if (reserve != nullptr && reserve->isAlive) allPlayersDead = false; } // Add this line!
    
         
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