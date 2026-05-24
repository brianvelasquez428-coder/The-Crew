#include "CombatUI.h" 
#include "CombatEngine.h" 
#include "AbilityDatabase.h"
#include "MoveDatabase.h" // Added to access getCategoryName()
#include "../Systems/Logger.h" 
#include "raylib.h" 
#include <string> 
#include <algorithm>

// --- ANIMATION TRACKERS ---
Entity* animAttacker = nullptr;
Vector2 animAttackerPos = {0,0};
Entity* animTarget = nullptr;
Vector2 animTargetOffset = {0,0};
Entity* animDying = nullptr;     
int deathFadeAlpha = 255;        

Vector2 GetBasePos(Entity* e, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam) {
    int pX = 200; for (Entity* p : pTeam) { if (p->isAlive || p == animDying || p == animTarget) { if (p == e) return {(float)pX, 300.0f}; pX += 250; } }
    int eX = 1600; for (Entity* en : eTeam) { if (en->isAlive || en == animDying || en == animTarget) { if (en == e) return {(float)eX, 300.0f}; eX -= 250; } }
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
    DrawRectangle(0, 0, 1920, 600, DARKGRAY);
    DrawText("MOMENTUM:", 860, 20, 25, YELLOW); DrawRectangle(860, 50, 200, 10, BLACK); DrawRectangle(860, 50, momentum * 2, 10, YELLOW);
    
    int pX = 200;
    for (Entity* p : playerTeam) {
        if (!p->isAlive && p != animDying && p != animTarget) continue; 
        Color hpColor = (p->currentHP > 0) ? GREEN : RED; 
        Color bodyColor = BLUE; Color textColor = WHITE; Color barBgColor = BLACK;
        
        if (p == animDying) { 
            float a = deathFadeAlpha / 255.0f;
            hpColor = Fade(hpColor, a); bodyColor = Fade(bodyColor, a);
            textColor = Fade(textColor, a); barBgColor = Fade(barBgColor, a);
        }
        float drawX = pX; float drawY = 300;
        if (p == animAttacker) { drawX = animAttackerPos.x; drawY = animAttackerPos.y; }
        if (p == animTarget) { drawX += animTargetOffset.x; drawY += animTargetOffset.y; }
        DrawRectangle(drawX, drawY, 100, 200, bodyColor); 
        DrawText(TextFormat("%s [Lv %d]", p->name.c_str(), p->level), drawX - 20, drawY - 60, 25, textColor);
        
        if (p->shieldHP > 0) {
            int shieldWidth = (p->shieldHP * 140) / p->maxHP;
            if (shieldWidth > 140) shieldWidth = 140; 
            DrawRectangle(drawX - 20, drawY - 45, 140, 10, barBgColor);
            DrawRectangle(drawX - 20, drawY - 45, shieldWidth, 10, SKYBLUE);
        }

        DrawRectangle(drawX - 20, drawY - 30, 140, 20, barBgColor); DrawRectangle(drawX - 20, drawY - 30, (p->currentHP * 140) / p->maxHP, 20, hpColor);
        DrawText(TextFormat("%d / %d", p->currentHP, p->maxHP), drawX - 20, drawY + 210, 20, textColor); 
        DrawText(TextFormat("STAM: %d", p->currentStamina), drawX - 20, drawY + 240, 20, Fade(LIGHTGRAY, textColor.a/255.0f));
        pX += 250;
    }
    
    int eX = 1600;
    for (Entity* e : enemyTeam) {
        if (!e->isAlive && e != animDying && e != animTarget) continue; 
        Color hpColor = RED; Color bodyColor = RED; Color textColor = WHITE; Color barBgColor = BLACK;
        if (e == animDying) { 
            float a = deathFadeAlpha / 255.0f;
            hpColor = Fade(hpColor, a); bodyColor = Fade(bodyColor, a);
            textColor = Fade(textColor, a); barBgColor = Fade(barBgColor, a);
        }
        float drawX = eX; float drawY = 300;
        if (e == animAttacker) { drawX = animAttackerPos.x; drawY = animAttackerPos.y; }
        if (e == animTarget) { drawX += animTargetOffset.x; drawY += animTargetOffset.y; }
        DrawRectangle(drawX, drawY, 100, 200, bodyColor); 
        DrawText(e->name.c_str(), drawX - 20, drawY - 60, 25, textColor);
        
        if (e->shieldHP > 0) {
            int shieldWidth = (e->shieldHP * 140) / e->maxHP;
            if (shieldWidth > 140) shieldWidth = 140; 
            DrawRectangle(drawX - 20, drawY - 45, 140, 10, barBgColor);
            DrawRectangle(drawX - 20, drawY - 45, shieldWidth, 10, SKYBLUE);
        }

        DrawRectangle(drawX - 20, drawY - 30, 140, 20, barBgColor); DrawRectangle(drawX - 20, drawY - 30, (e->currentHP * 140) / e->maxHP, 20, hpColor);
        DrawText(TextFormat("%d / %d", e->currentHP, e->maxHP), drawX - 20, drawY + 210, 20, textColor);
        eX -= 250;
    }
    DrawRectangle(960, 600, 960, 480, Fade(BLACK, 0.8f)); DrawRectangleLines(960, 600, 960, 480, DARKGRAY);
    std::vector<std::string> logs = GameLog::GetMessages(); int logY = 620;
    for (std::string& msg : logs) { DrawText(msg.c_str(), 980, logY, 20, LIGHTGRAY); logY += 30; }
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
        BeginDrawing(); ClearBackground(BLACK); DrawBattleOverlay(pTeam, eTeam, momentum); EndDrawing();
    }
}

void AnimateReturn(Entity* attacker, std::vector<Entity*>& pTeam, std::vector<Entity*>& eTeam, int momentum) {
    Vector2 start = animAttackerPos;
    Vector2 end = GetBasePos(attacker, pTeam, eTeam);
    for (int i=0; i<=10; i++) { 
        float t = i / 10.0f;
        animAttackerPos.x = start.x + (end.x - start.x) * t;
        animAttackerPos.y = start.y + (end.y - start.y) * t;
        BeginDrawing(); ClearBackground(BLACK); DrawBattleOverlay(pTeam, eTeam, momentum); EndDrawing();
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
        BeginDrawing(); ClearBackground(BLACK); DrawBattleOverlay(pTeam, eTeam, momentum);
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
        BeginDrawing(); ClearBackground(BLACK); DrawBattleOverlay(pTeam, eTeam, momentum); EndDrawing();
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
        BeginDrawing(); ClearBackground(BLACK); DrawBattleOverlay(pTeam, eTeam, momentum); EndDrawing();
    }
    animAttacker = nullptr;
}

void pauseForPlayer(bool wasPlayerTurn, std::vector<Entity*>& playerTeam, std::vector<Entity*>& enemyTeam, int momentum) {
    double entryTime = GetTime(); 
    while (!WindowShouldClose()) {
        BeginDrawing(); ClearBackground(BLACK); DrawBattleOverlay(playerTeam, enemyTeam, momentum);
        DrawRectangle(0, 600, 960, 480, Fade(BLACK, 0.8f)); DrawText("[ CLICK MOUSE OR PRESS ENTER TO CONTINUE ]", 100, 800, 30, YELLOW);
        EndDrawing();
        if (GetTime() - entryTime > 0.25) {
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsKeyReleased(KEY_ENTER)) break;
        }
    }
    double exitTime = GetTime();
    while (GetTime() - exitTime < 0.2 && !WindowShouldClose()) {
        BeginDrawing(); ClearBackground(BLACK); DrawBattleOverlay(playerTeam, enemyTeam, momentum); EndDrawing();
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
        BeginDrawing(); ClearBackground(BLACK); DrawBattleOverlay(playerTeam, enemyTeam, 0);
        DrawRectangle(0, 600, 960, 480, Fade(DARKPURPLE, 0.6f)); DrawRectangleLines(0, 600, 960, 480, PURPLE);
        DrawText(TextFormat("SELECT TARGET(S) FOR: %s (%d needed)", selectedMove.name.c_str(), targetsNeeded - selectedTargets.size()), 50, 630, 30, WHITE);
        DrawText("[ HOVER OVER A TARGET AND CLICK, OR PRESS 'B' TO CANCEL ]", 50, 700, 20, LIGHTGRAY); 
        
        int pX = 200; int eX = 1600;
        for (Entity* e : validPool) {
            if (e->isAlive) {
                Rectangle targetBox;
                if (targetsEnemies) { targetBox = { (float)eX - 20, 240, 140, 260 }; eX -= 250; } else { targetBox = { (float)pX - 20, 240, 140, 260 }; pX += 250; }
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
        BeginDrawing(); ClearBackground(BLACK); DrawBattleOverlay(playerTeam, enemyTeam, teamMomentum);
        DrawRectangle(0, 600, 960, 480, Fade(DARKBLUE, 0.5f)); DrawRectangleLines(0, 600, 960, 480, WHITE);
        
        DrawText(TextFormat("WHAT WILL %s DO?", character->name.c_str()), 50, 620, 40, WHITE);
        DrawText("[ Right-Click any Attack, or Right-Click a Character's HP Bar to Inspect! ]", 50, 665, 18, LIGHTGRAY);
        
        if (!showDetailsPopup) {
            int pX = 200; int eX = 1600;
            for (Entity* p : playerTeam) {
                if (!p->isAlive) continue; 
                if (CheckCollisionPointRec(GetMousePosition(), {(float)pX - 20, 240, 140, 260}) && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) { detailedEntity = p; detailsType = 5; showDetailsPopup = true; }
                pX += 250;
            }
            for (Entity* e : enemyTeam) {
                if (!e->isAlive) continue; 
                if (CheckCollisionPointRec(GetMousePosition(), {(float)eX - 20, 240, 140, 260}) && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) { detailedEntity = e; detailsType = 5; showDetailsPopup = true; }
                eX -= 250;
            }
        }
        
        if (!inItemMenu && !inReviveMenu && !inTeamUpMenu && !inSwapMenu) {
            int startX = 50; int startY = 700; int buttonKeys[4] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR};
            for (int i = 0; i < character->combatMenu.size(); i++) {
                Rectangle btn = { (float)startX, (float)startY + (i * 60), 400, 45 };
                if (DrawGUIButton(btn, (std::to_string(i+1) + ". " + character->combatMenu[i].name + " [" + std::to_string(character->combatMenu[i].staminaCost) + "]").c_str(), buttonKeys[i], showDetailsPopup)) {
                    if (character->currentStamina >= character->combatMenu[i].staminaCost) { selectedMove = character->combatMenu[i]; moveSelected = true; } else GameLog::Add("[!] Not enough stamina!");
                }
                if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && !showDetailsPopup) { detailedMove = character->combatMenu[i]; detailsType = 1; showDetailsPopup = true; }
            }
            if (DrawGUIButton({480, 700, 250, 45}, "5. Backpack", KEY_FIVE, showDetailsPopup)) inItemMenu = true;
            if (DrawGUIButton({480, 760, 250, 45}, "6. Team-Up", KEY_SIX, showDetailsPopup)) inTeamUpMenu = true;
            if (canFlee && DrawGUIButton({480, 820, 250, 45}, "7. Retreat", KEY_SEVEN, showDetailsPopup)) { GameLog::Add(">>> FELL BACK TO HIDEOUT <<<"); battleIsActive = false; turnComplete = true; }
            
            int nextBtnNum = 8;
            if (!character->altCombatMenu.empty()) {
                if (DrawGUIButton({480, 880, 250, 45}, TextFormat("%d. Stance (%s)", nextBtnNum, character->isAltStance ? "Strike" : "Support"), (nextBtnNum == 8 ? KEY_EIGHT : KEY_NINE), showDetailsPopup)) { 
                    character->toggleStance(); GameLog::Add(character->name + " shifted their combat stance!"); 
                }
                nextBtnNum++;
            }
            
            // --- NEW: CREW SWAP BUTTON ---
            if (DrawGUIButton({740, 700, 200, 45}, TextFormat("%d. Swap Crew", nextBtnNum), (nextBtnNum == 8 ? KEY_EIGHT : KEY_NINE), showDetailsPopup)) {
                inSwapMenu = true;
            }
        } 
        else if (inItemMenu && !inReviveMenu) {
            DrawText("BACKPACK:", 50, 700, 30, LIGHTGRAY); bool usedItem = false;
            if (DrawGUIButton({50, 740, 300, 45}, TextFormat("1. Bandage (x%d)", inventory[0]), KEY_ONE, showDetailsPopup)) { if (inventory[0] > 0) { inventory[0]--; character->currentHP += (character->maxHP * 0.3); if (character->currentHP > character->maxHP) character->currentHP = character->maxHP; GameLog::Add(character->name + " used a Bandage!"); usedItem = true; } else GameLog::Add("Out of Bandages!"); }
            if (DrawGUIButton({50, 800, 300, 45}, TextFormat("2. Energy Drink (x%d)", inventory[1]), KEY_TWO, showDetailsPopup)) { if (inventory[1] > 0) { inventory[1]--; character->currentStamina += 40; if (character->currentStamina > 200) character->currentStamina = 200; GameLog::Add(character->name + " drank an Energy Drink!"); usedItem = true; } else GameLog::Add("Out of Energy Drinks!"); }
            if (DrawGUIButton({50, 860, 300, 45}, TextFormat("3. Revive (x%d)", inventory[2]), KEY_THREE, showDetailsPopup)) { if (inventory[2] > 0) { bool anyoneDead = false; for (Entity* p : playerTeam) if (!p->isAlive) anyoneDead = true; if (anyoneDead) { inItemMenu = false; inReviveMenu = true; } else GameLog::Add("Everyone is already alive!"); } else GameLog::Add("Out of Revives!"); }
            if (DrawGUIButton({400, 740, 200, 45}, "4. Back [B]", KEY_FOUR, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) inItemMenu = false;
            if (usedItem) turnComplete = true; 
        }
        else if (inReviveMenu) {
            DrawText("WHO TO REVIVE?", 50, 700, 30, LIGHTGRAY);
            int startY = 740; int btnIndex = 0; int buttonKeys[3] = {KEY_ONE, KEY_TWO, KEY_THREE};
            for (Entity* p : playerTeam) {
                if (!p->isAlive) {
                    if (DrawGUIButton({ 50.0f, (float)startY + (btnIndex * 60), 300.0f, 45.0f }, (std::to_string(btnIndex + 1) + ". " + p->name).c_str(), buttonKeys[btnIndex], showDetailsPopup)) { inventory[2]--; p->isAlive = true; p->currentHP = p->maxHP / 2; GameLog::Add(character->name + " used a Revive on " + p->name + "!"); turnComplete = true; break; }
                    btnIndex++;
                }
            }
            if (DrawGUIButton({400, 740, 200, 45}, "4. Cancel [B]", KEY_FOUR, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) { inReviveMenu = false; inItemMenu = true; }
        }
        else if (inTeamUpMenu) {
            DrawText("TEAM-UPS:", 50, 700, 30, LIGHTGRAY);
            std::vector<TeamUpSkill> activeCombos = getActiveTeamUps(playerTeam, masterTeamUps);
            if (activeCombos.empty()) DrawText("No Team-Ups available.", 50, 750, 20, RED);
            else {
                int btnIndex = 0; int buttonKeys[4] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR};
                for (TeamUpSkill& combo : activeCombos) {
                    Rectangle btn = { 50.0f, 740.0f + (btnIndex * 60), 400.0f, 45.0f };
                    if (DrawGUIButton(btn, (std::to_string(btnIndex + 1) + ". " + combo.name + " [" + std::to_string(combo.momentumCost) + "%]").c_str(), buttonKeys[btnIndex], showDetailsPopup)) { if (teamMomentum >= combo.momentumCost) { selectedMove = combo.moveData; pendingMomentumCost = combo.momentumCost; moveSelected = true; } else GameLog::Add("[!] Not enough Momentum!"); }
                    btnIndex++;
                }
            }
            if (DrawGUIButton({480, 740, 200, 45}, "0. Cancel [B]", KEY_ZERO, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) inTeamUpMenu = false;
        }
        
        // --- NEW: COMBAT SWAP MENU ---
        else if (inSwapMenu) {
            DrawText("SWAP CREW MEMBERS", 50, 700, 30, LIGHTGRAY);
            DrawText("[ Click two slots to swap. Turn is NOT consumed! ]", 50, 735, 18, YELLOW);
            
            // Build temporary arrays so we never pass nullptr to the real party vectors
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
                Color btnColor = (swapGroup == g && swapIndex == i) ? ORANGE : DARKGRAY;
                
                if (DrawGUIButton(r, label.c_str(), 0, showDetailsPopup)) {
                    if (swapGroup == -1) { 
                        if (e != nullptr) { swapGroup = g; swapIndex = i; } 
                    } else { 
                        Entity* temp = getEntity(swapGroup, swapIndex);
                        
                        if (swapGroup == 0) tempActive[swapIndex] = getEntity(g, i);
                        else tempReserves[swapIndex] = getEntity(g, i);
                        
                        if (g == 0) tempActive[i] = temp;
                        else tempReserves[i] = temp;
                        
                        // Push only non-null entities back to the real vectors
                        playerTeam.clear();
                        for (auto* ent : tempActive) if (ent != nullptr) playerTeam.push_back(ent);
                        
                        masterRoster.clear();
                        for (auto* ent : tempReserves) if (ent != nullptr) masterRoster.push_back(ent);
                        
                        swapGroup = -1; swapIndex = -1;
                    }
                }
            };

            DrawText("ACTIVE:", 50, 765, 20, GREEN);
            for (int i=0; i < 3; i++) DrawSlot({50, 790.0f + (i * 45), 250, 40}, 0, i);

            DrawText("RESERVES:", 320, 765, 20, GOLD);
            for (int i=0; i < 3; i++) DrawSlot({320, 790.0f + (i * 45), 250, 40}, 1, i);

            if (DrawGUIButton({600, 790, 200, 45}, "0. Back [B]", KEY_ZERO, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) {
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
             BeginDrawing(); ClearBackground(BLACK); DrawBattleOverlay(playerTeam, enemyTeam, 0); EndDrawing();
        }
        while (!WindowShouldClose()) {
            BeginDrawing(); ClearBackground(BLACK); DrawBattleOverlay(playerTeam, enemyTeam, 0);
             
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
            BeginDrawing(); ClearBackground(BLACK); EndDrawing();
        }
        return true;
    } 
    return false;
}