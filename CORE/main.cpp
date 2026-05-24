#include "raylib.h"
#include "CombatManager.h"
#include "CharacterRoster.h"
#include "EnemyRoster.h"
#include "BossRoster.h"
#include "TeamUps.h"
#include "MoveDatabase.h" 
#include "AbilityDatabase.h" 
#include "../Systems/SaveManager.h" 
#include <string>
#include <ctime>
#include <algorithm>

enum GameScreen { TITLE, HUB, RESTING, STORE, PARTY, CUSTOMIZE, ENCYCLOPEDIA, BATTLE };

// --- TRANSLATION HELPERS ---
std::string getTargetText(MoveTarget t) { switch(t) { case MoveTarget::Self: return "Self"; case MoveTarget::OneEnemy: return "1 Enemy"; case MoveTarget::TwoEnemies: return "2 Enemies"; case MoveTarget::AllEnemies: return "All Enemies"; case MoveTarget::OneAlly: return "1 Ally"; case MoveTarget::TwoAllies: return "2 Allies"; case MoveTarget::AllAllies: return "All Allies"; default: return "Unknown"; } }
std::string getEffectText(Effect e) { switch(e) { case Effect::None: return "No special effect."; case Effect::RestoreStamina: return "Restores 25 Stamina."; case Effect::DefenseScalingDamage: return "Uses user's Defense stat to calculate damage."; case Effect::ApplyBleed: return "Applies Bleed (5 DMG/turn) for 3 turns."; case Effect::IgnoreDefense: return "Ignores 50% of the target's Defense."; case Effect::DefenseBuff40: return "Increases Defense by 40% and adds 1 Hit Nullification."; case Effect::HealAndCleanse: return "Heals 40% HP and cures all status conditions."; case Effect::LowerPriority: return "Reduces target's Speed by 15 for 2 turns."; case Effect::StrikeDefenseDebuff: return "Reduces target's Defense by 40% for 2 turns."; case Effect::StrikeUlt: return "Guaranteed Hit & Crit. Consumes stacks for massive damage."; case Effect::SlowEnemy: return "Reduces target's Speed by 20 for 2 turns."; case Effect::HoldLineShield: return "Applies a 60 HP Shield."; case Effect::PrecisionStrikeDebuff: return "Reduces target's DEF by 15 and BIQ by 10 for 3 turns."; case Effect::StaminaStrip: return "Halves target's Stamina Regeneration for 2 turns."; case Effect::P2BasicDebuff: return "Phase 2 Enhanced Basic Attack."; case Effect::ClutchGrab: return "Grabs target, preventing escape."; case Effect::LightsOutStun: return "Stuns the target for 1 turn."; default: return "Unknown effect."; } }

std::string getActorName(ActorID id) {
    switch(id) {
        case ActorID::Brian: return "Brian";
        case ActorID::Paul: return "Paul";
        case ActorID::Vince: return "Vince";
        case ActorID::Tony: return "Tony";
        case ActorID::Enemy: return "Enemy";
        default: return "Unknown";
    }
}

bool DrawMenuButton(Rectangle rect, const char* text, Color baseColor, bool disabled = false) {
    static double lastMenuClickTime = 0; 
    Vector2 mousePos = GetMousePosition();
    bool isHovered = CheckCollisionPointRec(mousePos, rect);
    if (disabled) isHovered = false; 

    DrawRectangleRec(rect, isHovered ? LIGHTGRAY : baseColor);
    DrawRectangleLinesEx(rect, 2, WHITE);
    DrawText(text, rect.x + 20, rect.y + 15, 20, isHovered ? BLACK : WHITE);
    
    if (!disabled && isHovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        if (GetTime() - lastMenuClickTime > 0.25) { 
            lastMenuClickTime = GetTime(); 
            return true; 
        }
    }
    return false;
}

void DrawDetailsOverlay(bool& showFlag, int type, Move m, std::string tuName, PassiveID pID, ActiveID aID, std::vector<TeamUpSkill>& masterTeamUps) {
    DrawRectangle(0, 0, 1920, 1080, Fade(BLACK, 0.95f));
    DrawRectangle(500, 300, 920, 480, DARKGRAY); DrawRectangleLinesEx({500, 300, 920, 480}, 4, WHITE);
    
    if (type == 1) { 
        DrawText(m.name.c_str(), 550, 350, 50, GOLD); 
        DrawText(TextFormat("TYPE: %s", getCategoryName(m.category).c_str()), 550, 420, 30, LIGHTGRAY);
        DrawText(TextFormat("STAMINA COST: %d", m.staminaCost), 550, 470, 30, GREEN); DrawText(TextFormat("HITS: %d", m.hitCount), 550, 520, 30, WHITE);
        DrawText(TextFormat("POWER MULTIPLIER: %.1fx", m.powerMultiplier), 550, 570, 30, ORANGE); DrawText(TextFormat("TARGETS: %s", getTargetText(m.target).c_str()), 550, 640, 30, SKYBLUE);
        DrawText(TextFormat("EFFECT: %s", getEffectText(m.effect).c_str()), 550, 690, 30, PURPLE);
    } 
    else if (type == 2) { 
        for (TeamUpSkill tu : masterTeamUps) {
            if (tu.name == tuName) {
                DrawText(tu.name.c_str(), 550, 350, 50, GOLD); DrawText(TextFormat("MOMENTUM COST: %d%%", tu.momentumCost), 550, 420, 30, YELLOW);
                DrawText(TextFormat("HITS: %d", tu.moveData.hitCount), 550, 470, 30, WHITE); DrawText(TextFormat("POWER MULTIPLIER: %.1fx", tu.moveData.powerMultiplier), 550, 520, 30, ORANGE);
                DrawText(TextFormat("TARGETS: %s", getTargetText(tu.moveData.target).c_str()), 550, 590, 30, SKYBLUE); DrawText(TextFormat("EFFECT: %s", getEffectText(tu.moveData.effect).c_str()), 550, 640, 30, PURPLE);
                break;
            }
        }
    }
    else if (type == 3) { DrawText(getPassiveName(pID).c_str(), 550, 350, 50, GOLD); DrawText("TYPE: PASSIVE", 550, 420, 30, LIGHTGRAY); DrawText("EFFECT:", 550, 490, 25, SKYBLUE); DrawText(getPassiveDescription(pID).c_str(), 550, 530, 25, WHITE); }
    else if (type == 4) { DrawText(getActiveName(aID).c_str(), 550, 350, 50, GOLD); DrawText("TYPE: ACTIVE", 550, 420, 30, LIGHTGRAY); DrawText("EFFECT:", 550, 490, 25, ORANGE); DrawText(getActiveDescription(aID).c_str(), 550, 530, 25, WHITE); }
    
    if (DrawMenuButton({550, 800, 300, 60}, "Close Info [B]", MAROON, false) || IsKeyReleased(KEY_B)) showFlag = false; 
}

int main() {
    srand(time(NULL)); InitWindow(1920, 1080, "The Crew - Pre-Alpha"); SetTargetFPS(60);

    std::vector<Entity*> activeParty(3, nullptr);
    std::vector<Entity*> reserves(3, nullptr);
    std::vector<Entity*> bench;
    
    activeParty[0] = new Entity(buildBrian());
    activeParty[1] = new Entity(buildPaul());
    activeParty[2] = new Entity(buildVince());
    bench.push_back(new Entity(buildYoungBrian()));
    bench.push_back(new Entity(buildYoungPaul()));
    bench.push_back(new Entity(buildYoungVince()));

    int swapGroup = -1; 
    int swapIndex = -1;

    std::vector<TeamUpSkill> masterTeamUps = buildMasterTeamUps();
    int inventory[3] = {5, 5, 5}; int wallet = 0; int selectedEncounter = 1; 

    Entity* selectedCharForCustomization = nullptr; bool showCustomSelect = false; int customSlotType = 0; int customSlotIndex = 0;
    
    bool showDetailsPopup = false; 
    int detailsType = 0; 
    Move detailedMove; 
    std::string detailedTeamUpName = ""; 
    PassiveID detailedPassive = PassiveID::None;
    ActiveID detailedActive = ActiveID::None;

    std::vector<MoveID> poolMoves = { 
        MoveID::None, MoveID::Strike, MoveID::TakeCover, MoveID::SupportStrike, MoveID::ClumsySwing, 
        MoveID::PaulBasic, MoveID::OneTwoPunch, MoveID::BobAndWeave, MoveID::KnifeSlash, MoveID::Haymaker, 
        MoveID::BrianStrikeBasic, MoveID::BrianStrikeSkill, MoveID::BrianStrikeUlt, MoveID::BrianSupportBasic, 
        MoveID::BrianSupportSkill, MoveID::BrianSupportUlt, MoveID::VinceBasic, MoveID::Shove, 
        MoveID::MiniSledge, MoveID::Sledgehammer, MoveID::HoldTheLine 
    };

    GameScreen currentScreen = TITLE;
    bool showSavePopup = false;

    while (!WindowShouldClose()) {
        bool bgDisabled = showDetailsPopup || showCustomSelect || showSavePopup; 

        if (currentScreen == TITLE) {
            BeginDrawing(); ClearBackground(BLACK); DrawText("THE CREW", 1920/2 - MeasureText("THE CREW", 80)/2, 200, 80, RED);
            bool hasSave = SaveSystem::DoesSaveExist(); int startX = 1920/2 - 200;
            if (hasSave) {
                if (DrawMenuButton({(float)startX, 400, 400, 60}, "1. Continue", DARKGRAY)) { SaveSystem::LoadGame(wallet, inventory); currentScreen = HUB; }
                if (DrawMenuButton({(float)startX, 480, 400, 60}, "2. New Game", DARKGRAY)) { wallet = 0; inventory[0] = 5; inventory[1] = 5; inventory[2] = 5; currentScreen = HUB; }
                if (DrawMenuButton({(float)startX, 560, 400, 60}, "3. Delete Save", MAROON)) SaveSystem::DeleteSave();
                if (DrawMenuButton({(float)startX, 640, 400, 60}, "4. Quit to Desktop", DARKGRAY)) break; 
            } else {
                if (DrawMenuButton({(float)startX, 400, 400, 60}, "1. New Game", DARKGRAY)) { wallet = 0; inventory[0] = 5; inventory[1] = 5; inventory[2] = 5; currentScreen = HUB; }
                if (DrawMenuButton({(float)startX, 480, 400, 60}, "2. Quit to Desktop", DARKGRAY)) break;
            }
            EndDrawing();
        }
        else if (currentScreen == HUB) {
            BeginDrawing(); ClearBackground(DARKBLUE); DrawText("THE HIDEOUT", 100, 100, 60, WHITE); DrawText(TextFormat("Crew Funds: $%d", wallet), 100, 200, 30, GREEN);
            if (DrawMenuButton({100, 300, 400, 60}, "1. Hit the Streets", GRAY, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_ONE))) { selectedEncounter = 1; currentScreen = BATTLE; }
            if (DrawMenuButton({100, 380, 400, 60}, "2. Visit the Bodega", GRAY, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_TWO))) currentScreen = STORE;
            
            if (DrawMenuButton({100, 460, 400, 60}, "3. Rest (Full Heal)", GOLD, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_THREE))) {
                for (Entity* p : activeParty) if(p) { p->currentHP = p->maxHP; p->currentStamina = p->maxStamina; p->isAlive = true; }
                for (Entity* p : reserves) if(p) { p->currentHP = p->maxHP; p->currentStamina = p->maxStamina; p->isAlive = true; }
                for (Entity* p : bench) if(p) { p->currentHP = p->maxHP; p->currentStamina = p->maxStamina; p->isAlive = true; }
                currentScreen = RESTING; 
            }

            if (DrawMenuButton({100, 540, 400, 60}, "4. Manage Crew", GRAY, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_FOUR))) currentScreen = PARTY;
            if (DrawMenuButton({100, 620, 400, 60}, "5. Fight Boss (Tony)", MAROON, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_FIVE))) { selectedEncounter = 2; currentScreen = BATTLE; }
            if (DrawMenuButton({100, 700, 400, 60}, "6. Training Sandbox", ORANGE, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_SIX))) { selectedEncounter = 3; currentScreen = BATTLE; }
            
            if (DrawMenuButton({100, 780, 400, 60}, "7. Save Progress", DARKGREEN, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_SEVEN))) { 
                SaveSystem::SaveGame(wallet, inventory); 
                showSavePopup = true; 
            }
            
            if (DrawMenuButton({100, 860, 400, 60}, "8. The Archives", PURPLE, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_EIGHT))) currentScreen = ENCYCLOPEDIA;
            if (DrawMenuButton({100, 940, 400, 60}, "9. Quit to Title", DARKGRAY, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_NINE))) currentScreen = TITLE;

            if (showSavePopup) {
                DrawRectangle(0, 0, 1920, 1080, Fade(BLACK, 0.8f)); 
                DrawRectangle(1920/2 - 250, 1080/2 - 150, 500, 300, DARKGRAY); 
                DrawRectangleLinesEx({1920/2 - 250, 1080/2 - 150, 500, 300}, 4, GREEN); 
                DrawText("GAME SAVED!", 1920/2 - MeasureText("GAME SAVED!", 40)/2, 1080/2 - 50, 40, GREEN);
                
                if (DrawMenuButton({1920/2 - 100, 1080/2 + 50, 200, 60}, "OK [Enter]", DARKGREEN, false) || IsKeyReleased(KEY_ENTER)) {
                    showSavePopup = false;
                }
            }
            EndDrawing();
        }
        else if (currentScreen == RESTING) {
            BeginDrawing(); ClearBackground(BLACK);
            DrawText("ZZZ...", 1920/2 - MeasureText("ZZZ...", 100)/2, 400, 100, SKYBLUE);
            DrawText("[ CLICK OR PRESS ENTER TO WAKE UP ]", 1920/2 - MeasureText("[ CLICK OR PRESS ENTER TO WAKE UP ]", 30)/2, 700, 30, GRAY);
            EndDrawing();
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsKeyReleased(KEY_ENTER)) currentScreen = HUB;
        }
        else if (currentScreen == STORE) {
            BeginDrawing(); ClearBackground(DARKGRAY); DrawText("LOCAL BODEGA", 100, 100, 60, ORANGE); DrawText(TextFormat("Wallet: $%d", wallet), 100, 180, 40, GREEN);
            if (DrawMenuButton({100, 300, 500, 60}, "1. Buy Bandage ($10)", (wallet >= 10) ? GRAY : RED) || IsKeyPressed(KEY_ONE)) { if (wallet >= 10) { wallet -= 10; inventory[0]++; } }
            if (DrawMenuButton({100, 380, 500, 60}, "2. Buy Energy Drink ($15)", (wallet >= 15) ? GRAY : RED) || IsKeyPressed(KEY_TWO)) { if (wallet >= 15) { wallet -= 15; inventory[1]++; } }
            if (DrawMenuButton({100, 460, 500, 60}, "3. Buy Revive ($30)", (wallet >= 30) ? GRAY : RED) || IsKeyPressed(KEY_THREE)) { if (wallet >= 30) { wallet -= 30; inventory[2]++; } }
            if (DrawMenuButton({100, 600, 300, 60}, "4. Back to Hub [B]", DARKGRAY) || IsKeyPressed(KEY_B)) currentScreen = HUB;
            EndDrawing();
        }
        else if (currentScreen == PARTY) {
            BeginDrawing(); ClearBackground(Fade(BLACK, 0.95f));
            DrawText("CREW MANAGEMENT", 100, 50, 50, PURPLE);
            DrawText("[ CLICK to Select/Swap | RIGHT-CLICK to Customize ]", 100, 110, 20, LIGHTGRAY);

            auto getEntity = [&](int g, int i) -> Entity* {
                if(g == 0) return activeParty[i]; else if(g == 1) return reserves[i]; else if (g == 2 && i < bench.size()) return bench[i]; return nullptr;
            };
            auto setEntity = [&](int g, int i, Entity* e) {
                if(g == 0) activeParty[i] = e; else if(g == 1) reserves[i] = e; else if(g == 2) { if(i >= bench.size()) bench.push_back(e); else bench[i] = e; }
            };

            auto DrawSlot = [&](Rectangle r, int g, int i) {
                Entity* e = getEntity(g, i);
                std::string label = e ? TextFormat("%s [Lv %d]", e->name.c_str(), e->level) : "[ EMPTY ]";
                Color btnColor = (swapGroup == g && swapIndex == i) ? ORANGE : DARKGRAY;
                
                if (DrawMenuButton(r, label.c_str(), btnColor, bgDisabled)) {
                    if (swapGroup == -1) { 
                        if (e != nullptr) { swapGroup = g; swapIndex = i; } 
                    } else { 
                        Entity* temp = getEntity(g, i);
                        setEntity(g, i, getEntity(swapGroup, swapIndex));
                        setEntity(swapGroup, swapIndex, temp);
                        swapGroup = -1; swapIndex = -1;
                        bench.erase(std::remove(bench.begin(), bench.end(), nullptr), bench.end()); 
                    }
                }
                if (e != nullptr && CheckCollisionPointRec(GetMousePosition(), r) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !bgDisabled) { 
                    selectedCharForCustomization = e; currentScreen = CUSTOMIZE; swapGroup = -1; 
                }
            };

            DrawText("ACTIVE:", 100, 160, 30, GREEN);
            for (int i=0; i<3; i++) DrawSlot({100, 200.0f + (i * 60), 300, 50}, 0, i);

            DrawText("RESERVES:", 450, 160, 30, YELLOW);
            for (int i=0; i<3; i++) DrawSlot({450, 200.0f + (i * 60), 300, 50}, 1, i);

            DrawText("BENCH:", 800, 160, 30, GRAY);
            for (int i=0; i<bench.size() + 1; i++) DrawSlot({800, 200.0f + (i * 60), 300, 50}, 2, i); 

            if (DrawMenuButton({100, 850, 300, 60}, "Back to Hub [B]", GRAY, bgDisabled) || IsKeyReleased(KEY_B)) { swapGroup = -1; currentScreen = HUB; }
            EndDrawing();
        }
        else if (currentScreen == CUSTOMIZE && selectedCharForCustomization != nullptr) {
            BeginDrawing(); ClearBackground(Fade(BLACK, 0.95f));
            DrawText(TextFormat("CUSTOMIZING: %s", selectedCharForCustomization->name.c_str()), 100, 30, 50, PURPLE);
            
            DrawText(TextFormat("HP: %d/%d  |  ATK: %d  |  DEF: %d  |  SPD: %d  |  STM: %d/%d  |  BIQ: %d  |  SIQ: %d", 
                selectedCharForCustomization->currentHP, selectedCharForCustomization->maxHP,
                selectedCharForCustomization->baseAttack, selectedCharForCustomization->baseDefense, 
                selectedCharForCustomization->baseSpeed, selectedCharForCustomization->currentStamina, 
                selectedCharForCustomization->maxStamina, selectedCharForCustomization->baseBIQ, 
                selectedCharForCustomization->baseSIQ), 100, 90, 20, LIGHTGRAY);

            int totalAbilitiesEquipped = selectedCharForCustomization->passiveAbilities.size() + selectedCharForCustomization->activeAbilities.size();

            DrawText("CURRENT MOVESET:", 100, 150, 30, GREEN);
            int yOffset = 200;
            for (int i = 0; i < selectedCharForCustomization->combatMenu.size(); i++) {
                Rectangle btn = { 100.0f, (float)yOffset, 450.0f, 50.0f };
                std::string text = selectedCharForCustomization->combatMenu[i].name + " [" + std::to_string(selectedCharForCustomization->combatMenu[i].staminaCost) + " STM]";
                
                if (DrawMenuButton(btn, text.c_str(), DARKGRAY, bgDisabled)) { customSlotType = 1; customSlotIndex = i; showCustomSelect = true; }
                if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !bgDisabled) { detailedMove = selectedCharForCustomization->combatMenu[i]; detailsType = 1; showDetailsPopup = true; }
                yOffset += 60;
            }
            if (selectedCharForCustomization->combatMenu.size() < 4) {
                if (DrawMenuButton({100.0f, (float)yOffset, 450.0f, 50.0f}, "+ Add Move", Fade(GREEN, 0.2f), bgDisabled)) {
                    Move placeholder = getMove(MoveID::Strike); selectedCharForCustomization->combatMenu.push_back(placeholder); customSlotType = 1; customSlotIndex = selectedCharForCustomization->combatMenu.size() - 1; showCustomSelect = true;
                }
            }

            DrawText(TextFormat("PASSIVE ABILITIES (%d/3 MAX):", totalAbilitiesEquipped), 650, 150, 30, SKYBLUE);
            yOffset = 200;
            if (selectedCharForCustomization->naturalAbility != PassiveID::None) {
                Rectangle natBtn = {650, (float)yOffset, 450, 50};
                DrawRectangleRec(natBtn, Fade(GOLD, 0.2f)); DrawRectangleLinesEx(natBtn, 2, GOLD);
                DrawText(TextFormat("[Natural] %s", getPassiveName(selectedCharForCustomization->naturalAbility).c_str()), 670, yOffset + 15, 20, GOLD);
                if (CheckCollisionPointRec(GetMousePosition(), natBtn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !bgDisabled) { detailedPassive = selectedCharForCustomization->naturalAbility; detailsType = 3; showDetailsPopup = true; }
                yOffset += 60;
            }
            for (int i = 0; i < selectedCharForCustomization->passiveAbilities.size(); i++) {
                Rectangle pBtn = { 650.0f, (float)yOffset, 450.0f, 50.0f };
                if (DrawMenuButton(pBtn, getPassiveName(selectedCharForCustomization->passiveAbilities[i]).c_str(), DARKGRAY, bgDisabled)) { customSlotType = 2; customSlotIndex = i; showCustomSelect = true; }
                if (CheckCollisionPointRec(GetMousePosition(), pBtn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !bgDisabled) { detailedPassive = selectedCharForCustomization->passiveAbilities[i]; detailsType = 3; showDetailsPopup = true; }
                yOffset += 60;
            }
            if (totalAbilitiesEquipped < 3) { 
                if (DrawMenuButton({650.0f, (float)yOffset, 450.0f, 50.0f}, "+ Add Passive", Fade(SKYBLUE, 0.2f), bgDisabled)) { selectedCharForCustomization->passiveAbilities.push_back(PassiveID::None); customSlotType = 2; customSlotIndex = selectedCharForCustomization->passiveAbilities.size() - 1; showCustomSelect = true; }
            }

            DrawText(TextFormat("ACTIVE ABILITIES (%d/1 MAX):", selectedCharForCustomization->activeAbilities.size()), 1200, 150, 30, ORANGE);
            yOffset = 200;
            for (int i = 0; i < selectedCharForCustomization->activeAbilities.size(); i++) {
                Rectangle aBtn = { 1200.0f, (float)yOffset, 450.0f, 50.0f };
                if (DrawMenuButton(aBtn, getActiveName(selectedCharForCustomization->activeAbilities[i]).c_str(), DARKGRAY, bgDisabled)) { customSlotType = 3; customSlotIndex = i; showCustomSelect = true; }
                if (CheckCollisionPointRec(GetMousePosition(), aBtn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !bgDisabled) { detailedActive = selectedCharForCustomization->activeAbilities[i]; detailsType = 4; showDetailsPopup = true; }
                yOffset += 60;
            }
            if (totalAbilitiesEquipped < 3 && selectedCharForCustomization->activeAbilities.size() < 1) {
                if (DrawMenuButton({1200.0f, (float)yOffset, 450.0f, 50.0f}, "+ Add Active", Fade(ORANGE, 0.2f), bgDisabled)) { selectedCharForCustomization->activeAbilities.push_back(ActiveID::None); customSlotType = 3; customSlotIndex = selectedCharForCustomization->activeAbilities.size() - 1; showCustomSelect = true; }
            }

            if (DrawMenuButton({100, 850, 300, 60}, "Back to Party [B]", DARKGRAY, bgDisabled) || (!bgDisabled && IsKeyReleased(KEY_B))) currentScreen = PARTY;

            if (showCustomSelect) {
                DrawRectangle(0, 0, 1920, 1080, Fade(BLACK, 0.95f)); DrawText("SELECT NEW ABILITY [Right-Click to Preview]", 100, 100, 50, WHITE);
                
                int gridX = 100; int gridY = 200;
                
                if (customSlotType == 1) { 
                    for (int i = 0; i < poolMoves.size(); i++) {
                        MoveID mID = poolMoves[i];
                        std::string label = (mID == MoveID::None) ? "--> UNEQUIP <--" : getMoveName(mID);
                        Rectangle btn = { (float)gridX, (float)gridY, 350.0f, 50.0f };
                        
                        if (DrawMenuButton(btn, label.c_str(), DARKGRAY, showDetailsPopup)) {
                            if (mID == MoveID::None) {
                                if (selectedCharForCustomization->combatMenu.size() > 1) 
                                    selectedCharForCustomization->combatMenu.erase(selectedCharForCustomization->combatMenu.begin() + customSlotIndex);
                            } else {
                                selectedCharForCustomization->combatMenu[customSlotIndex] = getMove(mID);
                            }
                            showCustomSelect = false;
                        }
                        if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !showDetailsPopup && mID != MoveID::None) {
                            detailedMove = getMove(mID); detailsType = 1; showDetailsPopup = true;
                        }
                        gridX += 370; if (gridX > 1500) { gridX = 100; gridY += 70; }
                    }
                } else if (customSlotType == 2) { 
                    for (int i = 0; i < MASTER_PASSIVE_POOL.size(); i++) {
                        PassiveID pID = MASTER_PASSIVE_POOL[i];
                        std::string label = (pID == PassiveID::None) ? "--> UNEQUIP <--" : getPassiveName(pID);
                        Rectangle btn = { (float)gridX, (float)gridY, 350.0f, 50.0f };
                        
                        if (DrawMenuButton(btn, label.c_str(), DARKGRAY, showDetailsPopup)) {
                            if (pID == PassiveID::None) selectedCharForCustomization->passiveAbilities.erase(selectedCharForCustomization->passiveAbilities.begin() + customSlotIndex);
                            else selectedCharForCustomization->passiveAbilities[customSlotIndex] = pID;
                            showCustomSelect = false;
                        }
                        if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !showDetailsPopup && pID != PassiveID::None) {
                            detailedPassive = pID; detailsType = 3; showDetailsPopup = true;
                        }
                        gridX += 370; if (gridX > 1500) { gridX = 100; gridY += 70; }
                    }
                } else if (customSlotType == 3) {
                    for (int i = 0; i < MASTER_ACTIVE_POOL.size(); i++) {
                        ActiveID aID = MASTER_ACTIVE_POOL[i];
                        std::string label = (aID == ActiveID::None) ? "--> UNEQUIP <--" : getActiveName(aID);
                        Rectangle btn = { (float)gridX, (float)gridY, 350.0f, 50.0f };
                        
                        if (DrawMenuButton(btn, label.c_str(), DARKGRAY, showDetailsPopup)) {
                            if (aID == ActiveID::None) selectedCharForCustomization->activeAbilities.erase(selectedCharForCustomization->activeAbilities.begin() + customSlotIndex);
                            else selectedCharForCustomization->activeAbilities[customSlotIndex] = aID;
                            showCustomSelect = false;
                        }
                        if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !showDetailsPopup && aID != ActiveID::None) {
                            detailedActive = aID; detailsType = 4; showDetailsPopup = true;
                        }
                        gridX += 370; if (gridX > 1500) { gridX = 100; gridY += 70; }
                    }
                }
                
                if (DrawMenuButton({100, 950, 300, 60}, "Cancel [B]", MAROON, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) showCustomSelect = false;
            }
            if (showDetailsPopup) DrawDetailsOverlay(showDetailsPopup, detailsType, detailedMove, detailedTeamUpName, detailedPassive, detailedActive, masterTeamUps);
            EndDrawing();
        }
        else if (currentScreen == ENCYCLOPEDIA) {
            static int activeTab = 0; BeginDrawing(); ClearBackground(DARKBLUE); DrawText("THE CREW ARCHIVES", 100, 50, 60, GOLD);
            if (DrawMenuButton({100, 160, 300, 60}, "1. Moves", (activeTab == 0) ? GOLD : GRAY, bgDisabled)) activeTab = 0;
            if (DrawMenuButton({100, 240, 300, 60}, "2. Passives", (activeTab == 1) ? GOLD : GRAY, bgDisabled)) activeTab = 1;
            if (DrawMenuButton({100, 320, 300, 60}, "3. Actives", (activeTab == 2) ? GOLD : GRAY, bgDisabled)) activeTab = 2;
            if (DrawMenuButton({100, 400, 300, 60}, "4. Team-Ups", (activeTab == 3) ? GOLD : GRAY, bgDisabled)) activeTab = 3;
            if (DrawMenuButton({100, 900, 300, 60}, "5. Back to Hub [B]", DARKGRAY, bgDisabled) || (!bgDisabled && IsKeyReleased(KEY_B))) currentScreen = HUB;

            DrawRectangle(450, 150, 1400, 800, Fade(BLACK, 0.8f)); DrawRectangleLines(450, 150, 1400, 800, WHITE);

            if (activeTab == 0) {
                int yPos = 250;
                for (int i = 1; i < poolMoves.size(); i++) {
                    int xPos = (i < 11) ? 480 : 1100; int yRender = (i < 11) ? yPos + ((i-1) * 60) : yPos + ((i - 11) * 60);
                    if (DrawMenuButton({ (float)xPos, (float)yRender, 550.0f, 50.0f }, getMoveName(poolMoves[i]).c_str(), DARKGRAY, bgDisabled)) { 
                        detailedMove = getMove(poolMoves[i]); detailsType = 1; showDetailsPopup = true; 
                    }
                }
            }
            else if (activeTab == 1) {
                int yPos = 250;
                for (int i = 1; i < MASTER_PASSIVE_POOL.size(); i++) { 
                    PassiveID pID = MASTER_PASSIVE_POOL[i];
                    DrawText(getPassiveName(pID).c_str(), 480, yPos, 25, YELLOW); 
                    DrawText(getPassiveDescription(pID).c_str(), 480, yPos + 30, 20, LIGHTGRAY); yPos += 80; 
                }
            }
            else if (activeTab == 2) {
                int yPos = 250;
                for (int i = 1; i < MASTER_ACTIVE_POOL.size(); i++) { 
                    ActiveID aID = MASTER_ACTIVE_POOL[i];
                    DrawText(getActiveName(aID).c_str(), 480, yPos, 25, ORANGE); 
                    DrawText(getActiveDescription(aID).c_str(), 480, yPos + 30, 20, LIGHTGRAY); yPos += 80; 
                }
            }
            else if (activeTab == 3) {
                int yPos = 250;
                for (TeamUpSkill tu : masterTeamUps) {
                    if (DrawMenuButton({ 480.0f, (float)yPos, 600.0f, 60.0f }, tu.name.c_str(), PURPLE, bgDisabled)) { detailedTeamUpName = tu.name; detailsType = 2; showDetailsPopup = true; }
                    
                    std::string reqs = "Crew Required: "; 
                    for (ActorID r : tu.requiredMembers) reqs += getActorName(r) + " "; 
                    
                    DrawText(reqs.c_str(), 1100, yPos + 20, 25, SKYBLUE); yPos += 80;
                }
            }
            if (showDetailsPopup) DrawDetailsOverlay(showDetailsPopup, detailsType, detailedMove, detailedTeamUpName, detailedPassive, detailedActive, masterTeamUps);
            EndDrawing();
        }
        else if (currentScreen == BATTLE) {
            std::vector<Entity*> enemyTeam; 
            
            std::vector<Entity*> fightParty; 
            for (Entity* p : activeParty) if (p != nullptr) fightParty.push_back(p);
            
            if (selectedEncounter == 1) { 
                Entity thug1 = buildStreetThug(); thug1.name = "Thug A"; enemyTeam.push_back(new Entity(thug1)); 
                Entity thug2 = buildScrawnyThug(); thug2.name = "Thug B"; enemyTeam.push_back(new Entity(thug2)); 
            } 
            else if (selectedEncounter == 2) { enemyTeam.push_back(new Entity(buildTony())); }
            else if (selectedEncounter == 3) { 
                int numEnemies = (rand() % 3) + 1; 
                
                for (int i = 0; i < numEnemies; i++) {
                    int enemyType = rand() % 2; 
                    
                    if (enemyType == 0) {
                        Entity thug = buildStreetThug(); 
                        thug.name = "Sandbox Thug " + std::to_string(i + 1); 
                        enemyTeam.push_back(new Entity(thug)); 
                    } else {
                        Entity punk = buildScrawnyThug(); 
                        punk.name = "Sandbox Punk " + std::to_string(i + 1); 
                        enemyTeam.push_back(new Entity(punk));
                    }
                }
            }

            // PASSING RESERVES INTO START BATTLE
            startBattle(reserves, fightParty, enemyTeam, masterTeamUps, inventory, wallet, true); 
            
            // RE-SYNC SWAPPED PARTY MEMBERS
            activeParty.clear();
            activeParty.resize(3, nullptr);
            for (size_t i = 0; i < fightParty.size() && i < 3; i++) {
                activeParty[i] = fightParty[i];
            }

            currentScreen = HUB;
        }
    }
    CloseWindow(); return 0;
}