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
#include <map> // <--- ADD THIS HEADER

// Removed the word "extern" so main.cpp officially owns this!
std::map<ActorID, Texture2D> globalSprites;

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

    // --- NEW: LOAD TEXTURES ---
    Texture2D texBrian = LoadTexture("Brian.png");
    Texture2D texPaul = LoadTexture("Paul.png");
    Texture2D texVince = LoadTexture("Vince.png");
    Texture2D texJoe = LoadTexture("Joe.png");
    Texture2D texJustin = LoadTexture("Justin.png");
    
    // Set to Point filtering so the pixel art doesn't blur when stretched
    SetTextureFilter(texBrian, TEXTURE_FILTER_POINT);
    SetTextureFilter(texPaul, TEXTURE_FILTER_POINT);
    SetTextureFilter(texVince, TEXTURE_FILTER_POINT);
    SetTextureFilter(texJoe, TEXTURE_FILTER_POINT);
    SetTextureFilter(texJustin, TEXTURE_FILTER_POINT);

    globalSprites[ActorID::Brian] = texBrian;
    globalSprites[ActorID::Paul] = texPaul;
    globalSprites[ActorID::Vince] = texVince;
    globalSprites[ActorID::Joe] = texJoe;
    globalSprites[ActorID::Justin] = texJustin;
    // --------------------------

    std::vector<Entity*> activeParty(3, nullptr);
    std::vector<Entity*> reserves(3, nullptr);
    std::vector<Entity*> bench;
    
    activeParty[0] = new Entity(buildYoungBrian());
    activeParty[1] = new Entity(buildYoungPaul());
    activeParty[2] = new Entity(buildYoungVince());

    reserves[0] = new Entity(buildYoungJoe());
    reserves[1] = new Entity(buildYoungJustin());
    
    bench.push_back(new Entity(buildBrian()));
    bench.push_back(new Entity(buildPaul()));
    bench.push_back(new Entity(buildVince()));

    int swapGroup = -1; 
    int swapIndex = -1;

    std::vector<TeamUpSkill> masterTeamUps = buildMasterTeamUps();
    int inventory[3] = {5, 5, 5}; int wallet = 0; int selectedEncounter = 1; 

    Entity* selectedCharForCustomization = nullptr; bool showCustomSelect = false; int customSlotType = 0; int customSlotIndex = 0;
    
    bool showDetailsPopup = false; 
    bool editingAltStance = false; // <-- ADD THIS
    int detailsType = 0; 
    Move detailedMove; 
    std::string detailedTeamUpName = ""; 
    PassiveID detailedPassive = PassiveID::None;
    ActiveID detailedActive = ActiveID::None;

    std::vector<MoveID> poolMoves = { 
        // Universal/Enemy moves
        MoveID::None, MoveID::Strike, MoveID::Guard, MoveID::ClumsySwing, 

        // Paul Moves Young
        MoveID::OneTwoPunch,
        // Paul Moves
        MoveID::PaulBasic, MoveID::BobAndWeave, MoveID::KnifeSlash, MoveID::Haymaker, 

        // Brian Moves Young (Support/Strike)
        MoveID::StepOff, MoveID::FaintPunches,
        // Brian Moves
        MoveID::BrianStrikeBasic, MoveID::BrianStrikeSkill, MoveID::BrianStrikeUlt, MoveID::BrianSupportBasic, 
        MoveID::BrianSupportSkill, MoveID::BrianSupportUlt, MoveID::VinceBasic, MoveID::Shove, 

        // Vince Moves
        MoveID::MiniSledge, MoveID::Sledgehammer, MoveID::HoldTheLine 
    };

    GameScreen currentScreen = TITLE;
    bool showSavePopup = false;

    while (!WindowShouldClose()) {
        bool bgDisabled = showDetailsPopup || showCustomSelect || showSavePopup; 

        if (currentScreen == TITLE) {
            BeginDrawing(); ClearBackground(BLACK); 
            
            DrawText("THE CREW", 1920/2 - MeasureText("THE CREW", 80)/2, 200, 80, RED);
            // ---> NEW: Version Subtitle <---
            DrawText("v1.1 Pre-Alpha", 1920/2 - MeasureText("v1.1 Pre-Alpha", 30)/2, 290, 30, GRAY);
            
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

            // --- NEW: DEBUG RESET BUTTON ---
            if (DrawMenuButton({1400, 940, 400, 60}, "[DEBUG] Reset Characters", MAROON, bgDisabled)) {
                // 1. Safely delete the old modified memory pointers
                for (Entity* p : activeParty) if (p) delete p;
                for (Entity* p : reserves) if (p) delete p;
                for (Entity* p : bench) if (p) delete p;

                // 2. Clear the vectors
                activeParty.clear(); activeParty.resize(3, nullptr);
                reserves.clear(); reserves.resize(3, nullptr);
                bench.clear();

                // 3. Rebuild everyone exactly as they are when the game launches
                activeParty[0] = new Entity(buildYoungBrian());
                activeParty[1] = new Entity(buildYoungPaul());
                activeParty[2] = new Entity(buildYoungVince());

                reserves[0] = new Entity(buildYoungJoe());
                reserves[1] = new Entity(buildYoungJustin());

                bench.push_back(new Entity(buildBrian()));
                bench.push_back(new Entity(buildPaul()));
                bench.push_back(new Entity(buildVince()));
            }
            // -------------------------------

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
                if (g == 0 && i < activeParty.size()) return activeParty[i];
                else if (g == 1 && i < reserves.size()) return reserves[i];
                else if (g == 2 && i < bench.size()) return bench[i];
                return nullptr;
            };
            
            auto setEntity = [&](int g, int i, Entity* e) {
                if (g == 0) { if (i >= activeParty.size()) activeParty.resize(3, nullptr); activeParty[i] = e; }
                else if (g == 1) { if (i >= reserves.size()) reserves.resize(3, nullptr); reserves[i] = e; }
                else if (g == 2) { if (i >= bench.size()) bench.push_back(e); else bench[i] = e; }
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
                    selectedCharForCustomization = e; currentScreen = CUSTOMIZE; swapGroup = -1; editingAltStance = false; // <-- ADDED HERE
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

            // 1. Profile Photo (Top Left next to name)
            if (globalSprites.count(selectedCharForCustomization->actorID)) {
                int textWidth = MeasureText(TextFormat("CUSTOMIZING: %s", selectedCharForCustomization->name.c_str()), 50);
                Rectangle profileDest = { (float)(100 + textWidth + 20), 20, 60, 60 };
                Rectangle sourceCrop = {0, 0, 7, 7}; // Top half of the sprite
                DrawTexturePro(globalSprites[selectedCharForCustomization->actorID], sourceCrop, profileDest, {0,0}, 0.0f, WHITE);
                DrawRectangleLinesEx(profileDest, 2, WHITE);
            }

            // 2. Full Body Picture & Backstory (Bottom Middle/Right)
            if (globalSprites.count(selectedCharForCustomization->actorID)) {
                Rectangle fullDest = { 950, 650, 140, 300 }; 
                
                // ---> NEW: Draw the DARKBLUE background box first
                Rectangle boxDest = { 930, 630, 180, 340 }; 
                DrawRectangleRec(boxDest, DARKBLUE);
                DrawRectangleLinesEx(boxDest, 2, WHITE);
                
                // Then draw the character sprite on top
                Rectangle fullSource = {0, 0, 7, 15}; 
                DrawTexturePro(globalSprites[selectedCharForCustomization->actorID], fullSource, fullDest, {0,0}, 0.0f, WHITE);

                std::string backstory = "A determined fighter trying to make a name in the streets.";
                if (selectedCharForCustomization->actorID == ActorID::Brian) backstory = "A natural talent who relies heavily on his sharp instincts.";
                else if (selectedCharForCustomization->actorID == ActorID::Paul) backstory = "A fierce brawler with an explosive temper and heavy hits.";
                else if (selectedCharForCustomization->actorID == ActorID::Vince) backstory = "The unwavering rock of the crew. Tough and unyielding.";
                else if (selectedCharForCustomization->actorID == ActorID::Joe) backstory = "YoOOOooo.";
                else if (selectedCharForCustomization->actorID == ActorID::Justin) backstory = "Your Favorite country white boy.";
                
                DrawText("CHARACTER BACKGROUND:", 1120, 750, 25, GOLD);
                DrawText(backstory.c_str(), 1120, 790, 20, LIGHTGRAY);
            }
            
            // --- NEW: QUICK LEVEL ADJUSTMENT FOR YOUNG CHARACTERS ---
            if (selectedCharForCustomization->currentStage == LifeStage::Young) {
                // MIN Level Button
                if (DrawMenuButton({ 710, 35, 60, 40 }, "MIN", DARKGRAY, bgDisabled)) {
                    while (selectedCharForCustomization->level > 1) {
                        selectedCharForCustomization->level--;
                        selectedCharForCustomization->expToNextLevel /= 1.5; // Reverse the EXP scaling
                        
                        EntityID id = selectedCharForCustomization->internalID;
                        if (id == EntityID::YoungBrian) { selectedCharForCustomization->maxHP -= 17; selectedCharForCustomization->baseAttack -= 4; selectedCharForCustomization->baseDefense -= 2; selectedCharForCustomization->baseSpeed -= 3; selectedCharForCustomization->baseBIQ -= 2; selectedCharForCustomization->baseSIQ -= 3; }
                        else if (id == EntityID::YoungPaul) { selectedCharForCustomization->maxHP -= 12; selectedCharForCustomization->baseAttack -= 3; selectedCharForCustomization->baseDefense -= 2; selectedCharForCustomization->baseSpeed -= 3; selectedCharForCustomization->baseBIQ -= 3; selectedCharForCustomization->baseSIQ -= 2; }
                        else if (id == EntityID::YoungVince) { selectedCharForCustomization->maxHP -= 20; selectedCharForCustomization->baseAttack -= 4; selectedCharForCustomization->baseDefense -= 3; selectedCharForCustomization->baseSpeed -= 1; selectedCharForCustomization->baseBIQ -= 2; selectedCharForCustomization->baseSIQ -= 1; }
                        else if (id == EntityID::YoungJoe) { selectedCharForCustomization->maxHP -= 14; selectedCharForCustomization->baseAttack -= 4; selectedCharForCustomization->baseDefense -= 1; selectedCharForCustomization->baseSpeed -= 3; selectedCharForCustomization->baseBIQ -= 2; selectedCharForCustomization->baseSIQ -= 2; }
                        else if (id == EntityID::YoungJustin) { selectedCharForCustomization->maxHP -= 19; selectedCharForCustomization->baseAttack -= 2; selectedCharForCustomization->baseDefense -= 3; selectedCharForCustomization->baseSpeed -= 3; selectedCharForCustomization->baseBIQ -= 2; selectedCharForCustomization->baseSIQ -= 1; }
                    }
                    selectedCharForCustomization->currentHP = selectedCharForCustomization->maxHP;
                    selectedCharForCustomization->maxStamina = selectedCharForCustomization->baseSIQ * 2;
                    selectedCharForCustomization->currentStamina = selectedCharForCustomization->maxStamina;
                    selectedCharForCustomization->calculateActiveStats();
                }

                // Level Down Button (-)
                if (DrawMenuButton({ 780, 35, 60, 40 }, "-", DARKGRAY, bgDisabled)) {
                    if (selectedCharForCustomization->level > 1) {
                        selectedCharForCustomization->level--;
                        selectedCharForCustomization->expToNextLevel /= 1.5; 
                        
                        EntityID id = selectedCharForCustomization->internalID;
                        if (id == EntityID::YoungBrian) { selectedCharForCustomization->maxHP -= 17; selectedCharForCustomization->baseAttack -= 4; selectedCharForCustomization->baseDefense -= 2; selectedCharForCustomization->baseSpeed -= 3; selectedCharForCustomization->baseBIQ -= 2; selectedCharForCustomization->baseSIQ -= 3; }
                        else if (id == EntityID::YoungPaul) { selectedCharForCustomization->maxHP -= 12; selectedCharForCustomization->baseAttack -= 3; selectedCharForCustomization->baseDefense -= 2; selectedCharForCustomization->baseSpeed -= 3; selectedCharForCustomization->baseBIQ -= 3; selectedCharForCustomization->baseSIQ -= 2; }
                        else if (id == EntityID::YoungVince) { selectedCharForCustomization->maxHP -= 20; selectedCharForCustomization->baseAttack -= 4; selectedCharForCustomization->baseDefense -= 3; selectedCharForCustomization->baseSpeed -= 1; selectedCharForCustomization->baseBIQ -= 2; selectedCharForCustomization->baseSIQ -= 1; }
                        else if (id == EntityID::YoungJoe) { selectedCharForCustomization->maxHP -= 14; selectedCharForCustomization->baseAttack -= 4; selectedCharForCustomization->baseDefense -= 1; selectedCharForCustomization->baseSpeed -= 3; selectedCharForCustomization->baseBIQ -= 2; selectedCharForCustomization->baseSIQ -= 2; }
                        else if (id == EntityID::YoungJustin) { selectedCharForCustomization->maxHP -= 19; selectedCharForCustomization->baseAttack -= 2; selectedCharForCustomization->baseDefense -= 3; selectedCharForCustomization->baseSpeed -= 3; selectedCharForCustomization->baseBIQ -= 2; selectedCharForCustomization->baseSIQ -= 1; }

                        selectedCharForCustomization->currentHP = selectedCharForCustomization->maxHP;
                        selectedCharForCustomization->maxStamina = selectedCharForCustomization->baseSIQ * 2;
                        selectedCharForCustomization->currentStamina = selectedCharForCustomization->maxStamina;
                        selectedCharForCustomization->calculateActiveStats();
                    }
                }
                
                // Display Current Level
                DrawText(TextFormat("Lv %d", selectedCharForCustomization->level), 865, 40, 30, YELLOW);
                
                // Level Up Button (+)
                if (DrawMenuButton({ 950, 35, 60, 40 }, "+", DARKGRAY, bgDisabled)) {
                    if (selectedCharForCustomization->level < 15) {
                        selectedCharForCustomization->levelUp();
                    }
                }

                // MAX Level Button
                if (DrawMenuButton({ 1020, 35, 60, 40 }, "MAX", DARKGRAY, bgDisabled)) {
                    while (selectedCharForCustomization->level < 15) {
                        selectedCharForCustomization->levelUp();
                    }
                    selectedCharForCustomization->currentHP = selectedCharForCustomization->maxHP;
                    selectedCharForCustomization->currentStamina = selectedCharForCustomization->maxStamina;
                }
            }
            // --------------------------------------------------------
            // --------------------------------------------------------
            
            // --- NEW: QUICK LEVEL ADJUSTMENT FOR YOUNG CHARACTERS ---
            // ... [Keep your +/- level logic here exactly as it is] ...
            // --------------------------------------------------------
            
            // --- NEW: STANCE STAT SHIFT PREVIEW ---
            int displayBIQ = selectedCharForCustomization->baseBIQ;
            int displaySIQ = selectedCharForCustomization->baseSIQ;
            
            // Spoof the stats to match toggleStance() if we are viewing the Alt Stance
            if (selectedCharForCustomization->naturalAbility == PassiveID::ScrewDat && editingAltStance) {
                displayBIQ = selectedCharForCustomization->baseSIQ; // <--- Now equals your actual base SIQ instead of 90!
                displaySIQ = 0;
            }
            
            DrawText(TextFormat("HP: %d/%d  |  ATK: %d  |  DEF: %d  |  SPD: %d  |  STM: %d/%d  |  BIQ: %d  |  SIQ: %d", 
                selectedCharForCustomization->currentHP, selectedCharForCustomization->maxHP,
                selectedCharForCustomization->baseAttack, selectedCharForCustomization->baseDefense, 
                selectedCharForCustomization->baseSpeed, selectedCharForCustomization->currentStamina, 
                selectedCharForCustomization->maxStamina, displayBIQ, displaySIQ), 100, 90, 20, LIGHTGRAY);

            int totalAbilitiesEquipped = selectedCharForCustomization->passiveAbilities.size() + selectedCharForCustomization->activeAbilities.size();

            // --- NEW: Stance Toggle Logic ---
            if (selectedCharForCustomization->actorID == ActorID::Brian) {
                DrawText(editingAltStance ? "ALT MOVESET (STRIKE):" : "MOVESET (SUPPORT):", 100, 150, 30, GREEN);
                
                // Pushed from X:440 to X:520 to give the text more breathing room, widened button slightly
                if (DrawMenuButton({520, 145, 130, 40}, editingAltStance ? "-> SUPPORT" : "-> STRIKE", DARKGRAY, bgDisabled)) {
                    editingAltStance = !editingAltStance;
                }
            } else {
                DrawText("CURRENT MOVESET:", 100, 150, 30, GREEN);
            }

            int yOffset = 200;
            // Tell the engine which array to look at and edit!
            std::vector<Move>& activeEditMenu = editingAltStance ? selectedCharForCustomization->altCombatMenu : selectedCharForCustomization->combatMenu;

            for (int i = 0; i < activeEditMenu.size(); i++) {
                Rectangle btn = { 100.0f, (float)yOffset, 450.0f, 50.0f };
                std::string text = activeEditMenu[i].name + " [" + std::to_string(activeEditMenu[i].staminaCost) + " STM]";
                
                if (DrawMenuButton(btn, text.c_str(), DARKGRAY, bgDisabled)) { customSlotType = 1; customSlotIndex = i; showCustomSelect = true; }
                if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !bgDisabled) { detailedMove = activeEditMenu[i]; detailsType = 1; showDetailsPopup = true; }
                yOffset += 60;
            }
            if (activeEditMenu.size() < 4) {
                if (DrawMenuButton({100.0f, (float)yOffset, 450.0f, 50.0f}, "+ Add Move", Fade(GREEN, 0.2f), bgDisabled)) {
                    Move placeholder = getMove(MoveID::Strike); activeEditMenu.push_back(placeholder); customSlotType = 1; customSlotIndex = activeEditMenu.size() - 1; showCustomSelect = true;
                }
            }
            // --------------------------------

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
                static int customScrollY = 0;

                DrawRectangle(0, 0, 1920, 1080, Fade(BLACK, 0.95f)); 
                DrawText("SELECT NEW ABILITY [Right-Click to Preview]", 100, 100, 50, WHITE);
                
                // --- SCROLL BOUNDARY LOGIC ---
                Rectangle scrollBounds = { 80, 180, 1760, 740 }; 

                // Calculate the max scroll limit dynamically based on items
                int totalContentHeight = 0;
                if (customSlotType == 1) {
                    totalContentHeight += 70; // Unequip button
                    std::vector<MoveCategory> categories = { MoveCategory::Basic, MoveCategory::Skill, MoveCategory::Ultimate, MoveCategory::Support };
                    for (MoveCategory cat : categories) {
                        totalContentHeight += 40; 
                        int itemsInCat = 0;
                        for (int i = 1; i < poolMoves.size(); i++) { if (getMove(poolMoves[i]).category == cat) itemsInCat++; }
                        totalContentHeight += ((itemsInCat + 3) / 4) * 60; 
                        totalContentHeight += 10;
                    }
                } else if (customSlotType == 2) {
                    totalContentHeight = ((MASTER_PASSIVE_POOL.size() + 3) / 4) * 70;
                } else if (customSlotType == 3) {
                    totalContentHeight = ((MASTER_ACTIVE_POOL.size() + 3) / 4) * 70;
                }
                
                int maxScroll = 0;
                if (totalContentHeight > 740) maxScroll = -(totalContentHeight - 740);

                if (customScrollY < maxScroll) customScrollY = maxScroll;
                if (customScrollY > 0) customScrollY = 0;

                // Capture Mouse Wheel
                if (CheckCollisionPointRec(GetMousePosition(), scrollBounds)) {
                    customScrollY += GetMouseWheelMove() * 50; 
                    if (customScrollY > 0) customScrollY = 0; 
                    if (customScrollY < maxScroll) customScrollY = maxScroll;
                }

                // Disable clicks on list items if hovering outside the box or if a detail popup is open
                bool listDisabled = showDetailsPopup || !CheckCollisionPointRec(GetMousePosition(), scrollBounds);
                
                BeginScissorMode(scrollBounds.x, scrollBounds.y, scrollBounds.width, scrollBounds.height);
                // -----------------------------

                int gridX = 100; 
                int gridY = 200 + customScrollY; // Inject the scroll offset here
                
                if (customSlotType == 1) { 
                    Rectangle unequipBtn = { 100.0f, (float)gridY, 350.0f, 50.0f };
                    if (DrawMenuButton(unequipBtn, "--> UNEQUIP <--", MAROON, listDisabled)) {
                        std::vector<Move>& targetMenu = editingAltStance ? selectedCharForCustomization->altCombatMenu : selectedCharForCustomization->combatMenu;
                        if (targetMenu.size() > 1) targetMenu.erase(targetMenu.begin() + customSlotIndex);
                        showCustomSelect = false; customScrollY = 0; // Reset scroll
                    }
                    gridY += 70; 

                    std::vector<MoveCategory> categories = { MoveCategory::Basic, MoveCategory::Skill, MoveCategory::Ultimate, MoveCategory::Support };
                    
                    for (MoveCategory cat : categories) {
                        DrawText(getCategoryName(cat).c_str(), 100, gridY, 30, SKYBLUE);
                        gridY += 40; gridX = 100;
                        
                        for (int i = 1; i < poolMoves.size(); i++) { 
                            MoveID mID = poolMoves[i];
                            Move m = getMove(mID);
                            
                            if (m.category == cat) {
                                Rectangle btn = { (float)gridX, (float)gridY, 350.0f, 50.0f };
                                
                                if (DrawMenuButton(btn, m.name.c_str(), DARKGRAY, listDisabled)) {
                                    std::vector<Move>& targetMenu = editingAltStance ? selectedCharForCustomization->altCombatMenu : selectedCharForCustomization->combatMenu;
                                    targetMenu[customSlotIndex] = getMove(mID);
                                    showCustomSelect = false; customScrollY = 0; // Reset scroll
                                }
                                if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !listDisabled) {
                                    detailedMove = getMove(mID); detailsType = 1; showDetailsPopup = true;
                                }
                                
                                gridX += 370; 
                                if (gridX > 1500) { gridX = 100; gridY += 60; }
                            }
                        }
                        if (gridX != 100) gridY += 60; 
                        gridY += 10; 
                    }
                } else if (customSlotType == 2) { 
                    for (int i = 0; i < MASTER_PASSIVE_POOL.size(); i++) {
                        PassiveID pID = MASTER_PASSIVE_POOL[i];
                        std::string label = (pID == PassiveID::None) ? "--> UNEQUIP <--" : getPassiveName(pID);
                        Rectangle btn = { (float)gridX, (float)gridY, 350.0f, 50.0f };
                        
                        if (DrawMenuButton(btn, label.c_str(), DARKGRAY, listDisabled)) {
                            if (pID == PassiveID::None) selectedCharForCustomization->passiveAbilities.erase(selectedCharForCustomization->passiveAbilities.begin() + customSlotIndex);
                            else selectedCharForCustomization->passiveAbilities[customSlotIndex] = pID;
                            showCustomSelect = false; customScrollY = 0; // Reset scroll
                        }
                        if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !listDisabled && pID != PassiveID::None) {
                            detailedPassive = pID; detailsType = 3; showDetailsPopup = true;
                        }
                        gridX += 370; if (gridX > 1500) { gridX = 100; gridY += 70; }
                    }
                } else if (customSlotType == 3) {
                    for (int i = 0; i < MASTER_ACTIVE_POOL.size(); i++) {
                        ActiveID aID = MASTER_ACTIVE_POOL[i];
                        std::string label = (aID == ActiveID::None) ? "--> UNEQUIP <--" : getActiveName(aID);
                        Rectangle btn = { (float)gridX, (float)gridY, 350.0f, 50.0f };
                        
                        if (DrawMenuButton(btn, label.c_str(), DARKGRAY, listDisabled)) {
                            if (aID == ActiveID::None) selectedCharForCustomization->activeAbilities.erase(selectedCharForCustomization->activeAbilities.begin() + customSlotIndex);
                            else selectedCharForCustomization->activeAbilities[customSlotIndex] = aID;
                            showCustomSelect = false; customScrollY = 0; // Reset scroll
                        }
                        if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !listDisabled && aID != ActiveID::None) {
                            detailedActive = aID; detailsType = 4; showDetailsPopup = true;
                        }
                        gridX += 370; if (gridX > 1500) { gridX = 100; gridY += 70; }
                    }
                }
                
                EndScissorMode(); // Stop clipping visuals
                
                // Cancel button stays out of the Scissor block so it remains pinned to the bottom
                if (DrawMenuButton({100, 950, 300, 60}, "Cancel [B]", MAROON, showDetailsPopup) || (!showDetailsPopup && IsKeyReleased(KEY_B))) {
                    showCustomSelect = false;
                    customScrollY = 0; // Reset scroll
                }
            }
            if (showDetailsPopup) DrawDetailsOverlay(showDetailsPopup, detailsType, detailedMove, detailedTeamUpName, detailedPassive, detailedActive, masterTeamUps);
            EndDrawing();
        }
        else if (currentScreen == ENCYCLOPEDIA) {
            static int activeTab = 0; 
            static int scrollY = 0; // Tracks scrolling position
            
            BeginDrawing(); ClearBackground(DARKBLUE); DrawText("THE CREW ARCHIVES", 100, 50, 60, GOLD);
            
            // Reset scroll to 0 whenever a new tab is clicked
            if (DrawMenuButton({100, 160, 300, 60}, "1. Moves", (activeTab == 0) ? GOLD : GRAY, bgDisabled)) { activeTab = 0; scrollY = 0; }
            if (DrawMenuButton({100, 240, 300, 60}, "2. Passives", (activeTab == 1) ? GOLD : GRAY, bgDisabled)) { activeTab = 1; scrollY = 0; }
            if (DrawMenuButton({100, 320, 300, 60}, "3. Actives", (activeTab == 2) ? GOLD : GRAY, bgDisabled)) { activeTab = 2; scrollY = 0; }
            if (DrawMenuButton({100, 400, 300, 60}, "4. Team-Ups", (activeTab == 3) ? GOLD : GRAY, bgDisabled)) { activeTab = 3; scrollY = 0; }
            if (DrawMenuButton({100, 900, 300, 60}, "5. Back to Hub [B]", DARKGRAY, bgDisabled) || (!bgDisabled && IsKeyReleased(KEY_B))) { currentScreen = HUB; scrollY = 0; }

            DrawRectangle(450, 150, 1400, 800, Fade(BLACK, 0.8f)); DrawRectangleLines(450, 150, 1400, 800, WHITE);

            // --- SCROLL LOGIC ---
            // Dynamically calculate encyclopedia heights to prevent infinite scrolling
            int totalContentHeight = 0;
            if (activeTab == 0) {
                totalContentHeight += 60;
                std::vector<MoveCategory> categories = { MoveCategory::Basic, MoveCategory::Skill, MoveCategory::Ultimate, MoveCategory::Support };
                for (MoveCategory cat : categories) {
                    totalContentHeight += 60;
                    for (int i = 1; i < poolMoves.size(); i++) { if (getMove(poolMoves[i]).category == cat) totalContentHeight += 60; }
                    totalContentHeight += 40; 
                }
            } else if (activeTab == 1) {
                totalContentHeight = 60 + (MASTER_PASSIVE_POOL.size() - 1) * 80;
            } else if (activeTab == 2) {
                totalContentHeight = 60 + (MASTER_ACTIVE_POOL.size() - 1) * 80;
            } else if (activeTab == 3) {
                totalContentHeight += 120;
                for (TeamUpSkill tu : masterTeamUps) if (tu.requiredMembers.size() == 3) totalContentHeight += 80;
                totalContentHeight += 100;
                for (TeamUpSkill tu : masterTeamUps) if (tu.requiredMembers.size() == 2) totalContentHeight += 80;
            }

            int maxScroll = 0;
            if (totalContentHeight > 800) maxScroll = -(totalContentHeight - 800);

            if (scrollY < maxScroll) scrollY = maxScroll;
            if (scrollY > 0) scrollY = 0;

            if (CheckCollisionPointRec(GetMousePosition(), {450, 150, 1400, 800})) {
                scrollY += GetMouseWheelMove() * 40; 
                if (scrollY > 0) scrollY = 0; 
                if (scrollY < maxScroll) scrollY = maxScroll;
            }
            
            // Disable clicks on list items if the mouse is outside the scissor box
            bool listDisabled = bgDisabled || !CheckCollisionPointRec(GetMousePosition(), {450, 150, 1400, 800});

            // Visually clip everything outside of this box
            BeginScissorMode(450, 150, 1400, 800);

            if (activeTab == 0) {
                int yPos = 180 + scrollY;
                
                std::vector<MoveCategory> categories = { MoveCategory::Basic, MoveCategory::Skill, MoveCategory::Ultimate, MoveCategory::Support };
                for (MoveCategory cat : categories) {
                    DrawText(getCategoryName(cat).c_str(), 480, yPos, 40, SKYBLUE); yPos += 60;
                    
                    for (int i = 1; i < poolMoves.size(); i++) {
                        Move m = getMove(poolMoves[i]);
                        if (m.category == cat) {
                            if (DrawMenuButton({ 480.0f, (float)yPos, 550.0f, 50.0f }, m.name.c_str(), DARKGRAY, listDisabled)) { 
                                detailedMove = m; detailsType = 1; showDetailsPopup = true; 
                            }
                            yPos += 60;
                        }
                    }
                    yPos += 40; // Spacing between categories
                }
            }
            else if (activeTab == 1) {
                int yPos = 180 + scrollY;
                for (int i = 1; i < MASTER_PASSIVE_POOL.size(); i++) { 
                    PassiveID pID = MASTER_PASSIVE_POOL[i];
                    DrawText(getPassiveName(pID).c_str(), 480, yPos, 25, YELLOW); 
                    DrawText(getPassiveDescription(pID).c_str(), 480, yPos + 30, 20, LIGHTGRAY); yPos += 80; 
                }
            }
            else if (activeTab == 2) {
                int yPos = 180 + scrollY;
                for (int i = 1; i < MASTER_ACTIVE_POOL.size(); i++) { 
                    ActiveID aID = MASTER_ACTIVE_POOL[i];
                    DrawText(getActiveName(aID).c_str(), 480, yPos, 25, ORANGE); 
                    DrawText(getActiveDescription(aID).c_str(), 480, yPos + 30, 20, LIGHTGRAY); yPos += 80; 
                }
            }
            else if (activeTab == 3) {
                int yPos = 180 + scrollY;
                
                // 3-Man Team-Ups
                DrawText("3-MAN TEAM-UPS", 480, yPos, 40, ORANGE); yPos += 60;
                for (TeamUpSkill tu : masterTeamUps) {
                    if (tu.requiredMembers.size() == 3) {
                        if (DrawMenuButton({ 480.0f, (float)yPos, 600.0f, 60.0f }, tu.name.c_str(), PURPLE, listDisabled)) { detailedTeamUpName = tu.name; detailsType = 2; showDetailsPopup = true; }
                        std::string reqs = "Crew Required: "; for (ActorID r : tu.requiredMembers) reqs += getActorName(r) + " "; 
                        DrawText(reqs.c_str(), 1100, yPos + 20, 25, SKYBLUE); yPos += 80;
                    }
                }
                
                yPos += 40; // Spacer
                
                // 2-Man Team-Ups
                DrawText("2-MAN TEAM-UPS", 480, yPos, 40, ORANGE); yPos += 60;
                for (TeamUpSkill tu : masterTeamUps) {
                    if (tu.requiredMembers.size() == 2) {
                        if (DrawMenuButton({ 480.0f, (float)yPos, 600.0f, 60.0f }, tu.name.c_str(), PURPLE, listDisabled)) { detailedTeamUpName = tu.name; detailsType = 2; showDetailsPopup = true; }
                        std::string reqs = "Crew Required: "; for (ActorID r : tu.requiredMembers) reqs += getActorName(r) + " "; 
                        DrawText(reqs.c_str(), 1100, yPos + 20, 25, SKYBLUE); yPos += 80;
                    }
                }
            }
            EndScissorMode(); // Stop clipping
            
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
            
            // ---> NEW: Re-sync Reserves to guarantee 3 slots <---
            reserves.resize(3, nullptr);
            
            currentScreen = HUB;
        }
    }
    UnloadTexture(texBrian);
    UnloadTexture(texPaul);
    UnloadTexture(texVince);
    UnloadTexture(texJoe);
    UnloadTexture(texJustin);
    CloseWindow(); return 0;
}