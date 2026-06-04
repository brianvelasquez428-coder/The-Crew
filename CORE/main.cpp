#include "raylib.h"
#include "CombatManager.h"
#include "CharacterRoster.h"
#include "EnemyRoster.h"
#include "BossRoster.h"
#include "TeamUps.h"
#include "MoveDatabase.h" 
#include "AbilityDatabase.h" 
#include "../Systems/SaveManager.h" 
#include "../CORE/GlobalConstants.h" // <--- Add this with your other includes!
#include <string>
#include <ctime>
#include <algorithm>
#include <map> // <--- ADD THIS HEADER

// ---> ADD THE NEW MAP DEFINITION HERE <---
std::map<ActorID, Texture2D> globalSprites;
std::map<ActorID, Texture2D> globalAttackSprites;

enum GameScreen { TITLE, HUB, MENU, RESTING, STORE, PARTY, CUSTOMIZE, ENCYCLOPEDIA, BATTLE, STORY_INTRO, EXPLORATION };

// --- TRANSLATION HELPERS ---

std::string getActorName(ActorID id) {
    switch(id) {
        case ActorID::Brian: return "Brian";
        case ActorID::Paul: return "Paul";
        case ActorID::Vince: return "Vince";
        case ActorID::Tony: return "Tony";
        case ActorID::ScrawnyThug: return "Scrawny Thug";
        case ActorID::StreetThug: return "Street Thug";
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

void DrawDetailsOverlay(bool& showFlag, int type, Move m, std::string tuName, PassiveID pID, ActiveID aID, NaturalID nID, std::vector<TeamUpSkill>& masterTeamUps) {
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
    
    else if (type == 7) { 
        DrawText(getNaturalName(nID).c_str(), 550, 350, 50, GOLD); 
        DrawText("TYPE: NATURAL", 550, 420, 30, LIGHTGRAY); 
        DrawText("EFFECT:", 550, 490, 25, SKYBLUE); 
        DrawText(getNaturalDescription(nID).c_str(), 550, 530, 25, WHITE); 
    }

    if (DrawMenuButton({550, 800, 300, 60}, "Close Info [B]", MAROON, false) || IsKeyReleased(KEY_B)) showFlag = false; 
}

int main() {
    srand(time(NULL)); InitWindow(1920, 1080, "The Crew - Pre-Alpha"); SetTargetFPS(60);

    // --- NEW: LOAD TEXTURES ---
    Texture2D texBrian = LoadTexture("BmanRightFaceRightLight.png");
    Texture2D texPaulIdle = LoadTexture("P_RightFaceRightLight.png");
    Texture2D texPaulPunch = LoadTexture("P_RightPunchRightLight.png");
    Texture2D texVince = LoadTexture("BobbyRightFaceRightLight.png");
    Texture2D texJoe = LoadTexture("Joe.png");
    Texture2D texJustin = LoadTexture("Justin.png");

    Texture2D texTony = LoadTexture("TonyLeftLight.png");

    Texture2D texBully = LoadTexture("Bully1LeftLight.png");
    Texture2D texThug = LoadTexture("Thug1LeftLight.png");
    
    // Set to Point filtering so the pixel art doesn't blur when stretched
    SetTextureFilter(texBrian, TEXTURE_FILTER_POINT);
    SetTextureFilter(texPaulIdle, TEXTURE_FILTER_POINT);
    SetTextureFilter(texPaulPunch, TEXTURE_FILTER_POINT);
    SetTextureFilter(texVince, TEXTURE_FILTER_POINT);
    SetTextureFilter(texJoe, TEXTURE_FILTER_POINT);
    SetTextureFilter(texJustin, TEXTURE_FILTER_POINT);

    SetTextureFilter(texTony, TEXTURE_FILTER_POINT);

    SetTextureFilter(texBully, TEXTURE_FILTER_POINT);
    SetTextureFilter(texThug, TEXTURE_FILTER_POINT);

    globalSprites[ActorID::Brian] = texBrian;
    globalSprites[ActorID::Paul] = texPaulIdle;
    globalAttackSprites[ActorID::Paul] = texPaulPunch;
    globalSprites[ActorID::Vince] = texVince;
    globalSprites[ActorID::Joe] = texJoe;
    globalSprites[ActorID::Justin] = texJustin;

    globalSprites[ActorID::Tony] = texTony;
    
    globalSprites[ActorID::ScrawnyThug] = texBully;
    globalSprites[ActorID::StreetThug] = texThug;


    std::vector<Entity*> activeParty(3, nullptr);
    std::vector<Entity*> reserves(3, nullptr);
    std::vector<Entity*> bench;
    
    activeParty[0] = new Entity(buildBrian());
    activeParty[1] = new Entity(buildPaul());
    activeParty[2] = new Entity(buildVince());

    reserves[0] = new Entity(buildYoungJoe());
    reserves[1] = new Entity(buildYoungJustin());
    
    bench.push_back(new Entity(buildYoungBrian()));
    bench.push_back(new Entity(buildYoungPaul()));
    bench.push_back(new Entity(buildYoungVince()));

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
    NaturalID detailedNatural = NaturalID::None;
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

    // --- STORY & EXPLORATION VARIABLES ---
    int storyCutsceneState = 0; 
    int playerFacing = 1;
    const int TILE_SIZE = 8; 

    // THE FIX: This remembers where we were before opening a sub-menu!
    GameScreen previousScreen = HUB;

    int currentActiveMap = 0; // 0 = Main Bar, 1 = Bathroom, 2 = Outside

    // --- MAP 0: THE MAIN BAR (40x18) ---
    int barMap[18][40] = {
        {1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1, 1,1,5,5,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, // 5 = North Door
        {1,1,1,1,1,1,1,1, 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1, 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1, 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,0,0,0,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1, 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,0,0,0,1,3,3,1,1,1,1}, // 3 = Bathroom
        {1,1,1,1,1,1,1,1, 1,1,2,2,2,2,2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,1,1,1,1,1,1, 1,1,2,2,2,2,2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,8,1}, // 8 = East Door
        {1,1,1,1,1,1,1,1, 1,1,0,0,0,0,0,0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,1}, // 8 = East Door
        {1,1,1,1,1,1,1,1, 1,1,0,0,0,0,0,0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,1,1,1,1,1,1, 1,1,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,0,0,0,0,0,1,1},
        {1,1,1,1,1,1,1,1, 1,1,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,0,0,0,0,0,1,1}, 
        {1,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,0,0,0,0,0,1,1}, 
        {1,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1}, 
        {1,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1}, 
        {1,1,1,1,1,1,1,1, 1,1,7,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, // 7 = South Door
        {1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1} 
    };

    // --- MAP 1: THE BATHROOM (12x12) ---
    int bathroomMap[12][12] = {
        {1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,1,1},
        {1,1,1,1,1,4,4,1,1,1,1,1}, // 4 = Door back to Bar!
        {1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1}
    };

    // --- MAP 2: OUTSIDE (50x30) ---
    int outsideMap[30][50] = {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,6,6,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // 6 = Enter North
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, 
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // 10 = Enter East
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // 10 = Enter East
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,9,9,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // 9 = Enter South
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    };

    // --- FIX: LAMBDA HELPER FUNCTION ---
    auto GetTileAt = [&](int gridX, int gridY) {
        if (currentActiveMap == 0) { // Main Bar
            if (gridX >= 0 && gridX < 40 && gridY >= 0 && gridY < 18) return barMap[gridY][gridX];
        } 
        else if (currentActiveMap == 1) { // Bathroom
            if (gridX >= 0 && gridX < 12 && gridY >= 0 && gridY < 12) return bathroomMap[gridY][gridX];
        }
        else if (currentActiveMap == 2) { // NEW: Outside Map
            if (gridX >= 0 && gridX < 50 && gridY >= 0 && gridY < 30) return outsideMap[gridY][gridX];
        }
        return 1; 
    };

    Vector2 playerWorldPos = { (float)(22 * TILE_SIZE), (float)(10 * TILE_SIZE) };
    float playerSpeed = 80.0f; 

    Camera2D exploreCamera = { 0 };
    exploreCamera.zoom = 6.0f; 
    exploreCamera.offset = { 1920 / 2.0f, 1080 / 2.0f };

    while (!WindowShouldClose()) {
        bool bgDisabled = showDetailsPopup || showCustomSelect || showSavePopup; 

        if (currentScreen == TITLE) {
            BeginDrawing(); ClearBackground(BLACK); 
            
            DrawText("THE CREW", 1920/2 - MeasureText("THE CREW", 80)/2, 200, 80, RED);
            // ---> NEW: Version Subtitle <---
            DrawText("v1.3 Pre-Alpha", 1920/2 - MeasureText("v1.3 Pre-Alpha", 30)/2, 290, 30, GRAY);
            
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
            // Inside the currentScreen == HUB block:
            if (DrawMenuButton({100, 300, 400, 60}, "0. Enter Story Mode", GOLD, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_ZERO))) { 
                storyCutsceneState = 0; 
                currentActiveMap = 0; // <--- FIX: Always reset to the Main Bar!
                currentScreen = STORY_INTRO; 
            }
            // Note: You may need to shift your other Hub buttons down by 80 pixels so they don't overlap!
            if (DrawMenuButton({100, 380, 400, 60}, "1. Hit the Streets", GRAY, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_ONE))) { selectedEncounter = 1; currentScreen = BATTLE; }
            if (DrawMenuButton({100, 460, 400, 60}, "2. Visit The Bodega", GOLD, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_TWO))) { 
                previousScreen = HUB; // <--- ADD THIS
                currentScreen = STORE; 
            }
            
            if (DrawMenuButton({100, 540, 400, 60}, "3. Rest (Full Heal)", GOLD, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_THREE))) {
                for (Entity* p : activeParty) if(p) { p->currentHP = p->maxHP; p->currentStamina = p->maxStamina; p->isAlive = true; }
                for (Entity* p : reserves) if(p) { p->currentHP = p->maxHP; p->currentStamina = p->maxStamina; p->isAlive = true; }
                for (Entity* p : bench) if(p) { p->currentHP = p->maxHP; p->currentStamina = p->maxStamina; p->isAlive = true; }
                currentScreen = RESTING; 
            }

            if (DrawMenuButton({100, 620, 400, 60}, "4. Manage Party", GRAY, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_ONE))) { 
                previousScreen = HUB; // <--- ADD THIS
                currentScreen = PARTY; 
            }
            if (DrawMenuButton({100, 700, 400, 60}, "5. Fight Boss (Tony)", MAROON, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_FIVE))) { selectedEncounter = 2; currentScreen = BATTLE; }
            if (DrawMenuButton({100, 780, 400, 60}, "6. Training Sandbox", ORANGE, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_SIX))) { selectedEncounter = 3; currentScreen = BATTLE; }
            
            if (DrawMenuButton({100, 860, 400, 60}, "7. Save Progress", DARKGREEN, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_SEVEN))) { 
                SaveSystem::SaveGame(wallet, inventory); 
                showSavePopup = true; 
            }
            
            // Replace your existing "8. The Archives" line in the HUB with this:
            if (DrawMenuButton({100, 940, 400, 60}, "8. The Archives", PURPLE, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_EIGHT))) {
                previousScreen = HUB;
                currentScreen = ENCYCLOPEDIA;
            }
            if (DrawMenuButton({100, 1020, 400, 60}, "9. Quit to Title", DARKGRAY, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_NINE))) currentScreen = TITLE;

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
        else if (currentScreen == MENU) { 
            BeginDrawing();
            
            // Faded black background so the game is still visible behind it
            ClearBackground(Fade(BLACK, 0.85f)); 

            DrawText("QUICK MENU", 100, 50, 40, WHITE);
            DrawText("[ Choose an option or press TAB to Resume ]", 100, 100, 20, LIGHTGRAY);

            // 1. Resume Game 
            if (DrawMenuButton({100, 150, 400, 60}, "Resume Game", GREEN, bgDisabled) || (!bgDisabled && IsKeyReleased(KEY_TAB))) { 
                currentScreen = EXPLORATION; 
            }
            
            // 2. Manage Party 
            if (DrawMenuButton({100, 230, 400, 60}, "Manage Party", ORANGE, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_TWO))) { 
                previousScreen = MENU; 
                currentScreen = PARTY; 
            }
            
            // 3. Store
            if (DrawMenuButton({100, 310, 400, 60}, "Visit The Bodega", GOLD, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_THREE))) { 
                previousScreen = MENU; 
                currentScreen = STORE; 
            }
            
            // 4. Archive 
            if (DrawMenuButton({100, 390, 400, 60}, "The Archive", SKYBLUE, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_FOUR))) {
                previousScreen = MENU;
                currentScreen = ENCYCLOPEDIA;
            }
            
            // 5. Save Game 
            if (DrawMenuButton({100, 470, 400, 60}, "Save Game", LIGHTGRAY, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_FIVE))) {
                SaveSystem::SaveGame(wallet, inventory); 
                showSavePopup = true; 
            }

            // 6. Return to Hub
            if (DrawMenuButton({100, 550, 400, 60}, "Return to Hub", DARKPURPLE, bgDisabled) || (!bgDisabled && IsKeyPressed(KEY_SIX))) {
                currentScreen = HUB;
            }
            
            // Draw Save Popup if active
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
        else if (currentScreen == STORY_INTRO) {
            if (storyCutsceneState == 0) {
                BeginDrawing(); ClearBackground(BLACK);
                DrawText("One day, at a local bar...", 1920/2 - MeasureText("One day, at a local bar...", 40)/2, 1080/2, 40, WHITE);
                DrawText("[ CLICK TO CONTINUE ]", 1920/2 - MeasureText("[ CLICK TO CONTINUE ]", 20)/2, 1080/2 + 60, 20, GRAY);
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsKeyReleased(KEY_ENTER)) storyCutsceneState++;
                EndDrawing();
            } 
            else {
                // --- 1. CUTSCENE CAMERA LOGIC ---
                // Center camera roughly around row 10, col 14
                Vector2 focusPos = { (float)(14 * TILE_SIZE) + 4.0f, (float)(10 * TILE_SIZE) + 12.0f }; 
                if (activeParty[0] != nullptr) {
                    if (activeParty[0]->actorID == ActorID::Paul) focusPos = { (float)(18 * TILE_SIZE) + 4.0f, (float)(10 * TILE_SIZE) + 12.0f };
                    else if (activeParty[0]->actorID == ActorID::Vince) focusPos = { (float)(10 * TILE_SIZE) + 4.0f, (float)(10 * TILE_SIZE) + 12.0f };
                }
                exploreCamera.target = focusPos;

                // --- SMART CAMERA CLAMP ---
                float minX = 0; float minY = 0;
                float maxX = 40 * TILE_SIZE; float maxY = 18 * TILE_SIZE;
                float viewWidth = 1920.0f / exploreCamera.zoom; float viewHeight = 1080.0f / exploreCamera.zoom;

                if ((maxX - minX) < viewWidth) {
                    exploreCamera.target.x = minX + ((maxX - minX) / 2.0f);
                } else {
                    if (exploreCamera.target.x - (viewWidth / 2.0f) < minX) exploreCamera.target.x = minX + (viewWidth / 2.0f);
                    if (exploreCamera.target.x + (viewWidth / 2.0f) > maxX) exploreCamera.target.x = maxX - (viewWidth / 2.0f);
                }

                if ((maxY - minY) < viewHeight) {
                    exploreCamera.target.y = minY + ((maxY - minY) / 2.0f);
                } else {
                    if (exploreCamera.target.y - (viewHeight / 2.0f) < minY) exploreCamera.target.y = minY + (viewHeight / 2.0f);
                    if (exploreCamera.target.y + (viewHeight / 2.0f) > maxY) exploreCamera.target.y = maxY - (viewHeight / 2.0f);
                }

                // --- 2. DRAW THE SCENE ---
                BeginDrawing(); ClearBackground(BLACK);
                
                // CRITICAL: This is what applies your 6.0x zoom to the map!
                BeginMode2D(exploreCamera); 

                for (int y = 0; y < 18; y++) {
                    for (int x = 0; x < 40; x++) {
                        Rectangle tileRec = { (float)(x * TILE_SIZE), (float)(y * TILE_SIZE), (float)TILE_SIZE, (float)TILE_SIZE };
                        int tileVal = barMap[y][x]; 
                        
                        if (tileVal == 1) { 
                            DrawRectangleRec(tileRec, DARKGRAY); DrawRectangleLinesEx(tileRec, 1, BLACK);
                        } else if (tileVal == 2) { 
                            DrawRectangleRec(tileRec, BROWN); 
                        } else if (tileVal == 3 || tileVal == 4) { 
                            DrawRectangleRec(tileRec, DARKBLUE); 
                        } else if (tileVal >= 5 && tileVal <= 10) {
                            DrawRectangleRec(tileRec, ORANGE);   
                        } else { 
                            DrawRectangleRec(tileRec, LIGHTGRAY);
                        }
                    }
                }

                // --- 3. DRAW THE CHARACTERS ---
                Texture2D texBrian = globalSprites[ActorID::Brian];
                Texture2D texPaul = globalSprites[ActorID::Paul];
                Texture2D texVince = globalSprites[ActorID::Vince];
                
                // Establish a solid floor line
                float groundY = (10 * TILE_SIZE) + 25.0f; 
                float standardWidth = 10.0f;

                // Helper lambda to cleanly draw and anchor cutscene characters
                auto DrawCutsceneSprite = [&](Texture2D tex, float xPos, float yPosFloor, int facing) {
                    float wOffset = (standardWidth - tex.width) / 2.0f;
                    Rectangle src = {0, 0, (float)tex.width * facing, (float)tex.height};
                    Rectangle dest = {xPos + wOffset, yPosFloor - tex.height, (float)tex.width, (float)tex.height};
                    DrawTexturePro(tex, src, dest, {0,0}, 0.0f, WHITE);
                };

                DrawCutsceneSprite(texBrian, 14 * TILE_SIZE, groundY, 1);
                DrawCutsceneSprite(texPaul, 18 * TILE_SIZE, groundY, -1);
                DrawCutsceneSprite(texVince, 10 * TILE_SIZE, groundY, 1);

                EndMode2D(); 

                // --- 4. DRAW THE UI ---
                DrawRectangle(360, 800, 1200, 200, Fade(BLACK, 0.8f)); 
                DrawRectangleLinesEx({360, 800, 1200, 200}, 4, WHITE);
                
                // --- TYPEWRITER STATE VARIABLES ---
                static int charsRevealed = 0;
                static int lastCutsceneState = -1;
                static double textRevealTimer = 0.0;
                
                // Reset the typewriter when the dialogue advances
                if (storyCutsceneState != lastCutsceneState) {
                    charsRevealed = 0;
                    lastCutsceneState = storyCutsceneState;
                    textRevealTimer = GetTime();
                }
                
                // Increase revealed characters over time (0.02s per char is a standard speed)
                if (GetTime() - textRevealTimer > 0.02) {
                    charsRevealed++;
                    textRevealTimer = GetTime();
                }

                // Helper Function: Formats text dynamically AND applies the typewriter limit
                auto DrawDialogue = [](const char* name, Color nameColor, const char* dialogue, int revealedLimit) -> int {
                    DrawText(name, 400, 830, 30, nameColor);
                    
                    int textX = 400; int textY = 880; int fontSize = 25;
                    int maxLineWidth = 1120; 
                    
                    std::string textStr(dialogue);
                    std::vector<std::string> lines;
                    std::string currentLine = "";
                    std::string word = "";
                    
                    // 1. Calculate the final paragraph layout behind the scenes
                    for (size_t i = 0; i <= textStr.length(); i++) {
                        if (i == textStr.length() || textStr[i] == ' ') {
                            std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
                            
                            if (MeasureText(testLine.c_str(), fontSize) > maxLineWidth) {
                                lines.push_back(currentLine + " "); // Lock in the line with a trailing space
                                currentLine = word;
                            } else {
                                currentLine = testLine;
                            }
                            word = "";
                        } else {
                            word += textStr[i];
                        }
                    }
                    if (!currentLine.empty()) lines.push_back(currentLine);

                    // 2. Draw the text line-by-line, stopping at the typewriter limit
                    int charsLeft = revealedLimit;
                    for (const std::string& line : lines) {
                        if (charsLeft <= 0) break;
                        
                        std::string lineToDraw = line;
                        if (lineToDraw.length() > charsLeft) {
                            lineToDraw = lineToDraw.substr(0, charsLeft);
                        }
                        
                        DrawText(lineToDraw.c_str(), textX, textY, fontSize, WHITE);
                        textY += fontSize + 10;
                        charsLeft -= line.length();
                    }
                    
                    // Return the total string length so the click handler knows when it's done typing!
                    return textStr.length(); 
                };

                // Track the length of the current line for the click logic
                int totalCharsInLine = 0;

                if (storyCutsceneState == 1) {
                    totalCharsInLine = DrawDialogue("Vince:", SKYBLUE, "Something's off... The prices here jumped up a dollar from before! The waitresses also don't feel right.", charsRevealed);
                } else if (storyCutsceneState == 2) {
                    totalCharsInLine = DrawDialogue("Brian:", YELLOW, "Dawg, how do you even notice that? we ain't been here in a minute?", charsRevealed);
                } else if (storyCutsceneState == 3) {
                    totalCharsInLine = DrawDialogue("Paul:", RED, "Keep a cool head. Let's look around for clues first before we do anything we're going to regret. We don't need another bounty on our heads.", charsRevealed);
                } else if (storyCutsceneState == 4) {
                    totalCharsInLine = DrawDialogue("Brian:", YELLOW, "Bro, the fuck you mean 'our'?", charsRevealed);
                } else if (storyCutsceneState == 5) {
                    totalCharsInLine = DrawDialogue("Paul:", RED, "Trust.", charsRevealed);
                }
                
                // Blink the [CLICK] prompt when typing is done
                if (charsRevealed >= totalCharsInLine) {
                    DrawText("[ CLICK ]", 1450, 950, 20, GRAY);
                }

                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsKeyReleased(KEY_ENTER)) {
                    // If it's still typing, clicking instantly fills the box
                    if (charsRevealed < totalCharsInLine) {
                        charsRevealed = totalCharsInLine;
                    } 
                    // If it's done typing, clicking advances the cutscene
                    else {
                        storyCutsceneState++;
                        if (storyCutsceneState > 5) {
                            if (activeParty[0] != nullptr && activeParty[0]->actorID == ActorID::Paul) {
                                playerWorldPos = { 18.0f * TILE_SIZE, 10.0f * TILE_SIZE };
                                playerFacing = -1;
                            }
                            else if (activeParty[0] != nullptr && activeParty[0]->actorID == ActorID::Vince) {
                                playerWorldPos = { 10.0f * TILE_SIZE, 10.0f * TILE_SIZE };
                                playerFacing = 1; 
                            }
                            else {
                                playerWorldPos = { 14.0f * TILE_SIZE, 10.0f * TILE_SIZE };
                                playerFacing = 1; 
                            }
                            currentScreen = EXPLORATION;
                        }
                    }
                }
                EndDrawing();
            }
        }
        else if (currentScreen == EXPLORATION) {
            float dt = GetFrameTime(); 
            
            // --- NEW: SPRINT LOGIC ---
            float currentSpeed = 50.0f; // New, slightly slower walking speed
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                currentSpeed = 110.0f; // Fast sprint speed!
            }

            float moveX = 0; float moveY = 0;

            // Gather the raw input math first
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) moveY -= currentSpeed * dt;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) moveY += currentSpeed * dt;
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) moveX -= currentSpeed * dt;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) moveX += currentSpeed * dt;

            // THE FIX: Only update the visual facing direction if they are actively moving left or right!
            // If they hold both, moveX is 0, so this gets skipped and they keep facing their original direction.
            if (moveX < 0) {
                playerFacing = -1;
            } else if (moveX > 0) {
                playerFacing = 1;
            }

            Texture2D playerTex = globalSprites[ActorID::Brian];
            if (activeParty[0] != nullptr && globalSprites.count(activeParty[0]->actorID)) {
                playerTex = globalSprites[activeParty[0]->actorID];
            }
            float pWidth = (float)playerTex.width;
            float pHeight = (float)playerTex.height;

            // --- THE FIX: UNIVERSAL PHYSICS FOOTPRINT ---
            float standardWidth = 10.0f;  // Universal base width
            float standardHeight = 25.0f; // Universal base height
            
            float hitBoxW = 6.0f; // The actual physics width
            float hitBoxH = 4.0f; // The actual physics height (just the feet)

            // Anchor the hitbox horizontally to the center, and vertically to the floor
            Rectangle playerHitBox = {
                playerWorldPos.x + (standardWidth / 2.0f) - (hitBoxW / 2.0f),
                playerWorldPos.y + standardHeight - hitBoxH,
                hitBoxW,
                hitBoxH
            };

            // --- AREA-BASED COLLISION ---
            auto isSolidArea = [&](Rectangle box) {
                int leftTile = box.x / TILE_SIZE;
                int rightTile = (box.x + box.width - 0.01f) / TILE_SIZE;
                int topTile = box.y / TILE_SIZE;
                int bottomTile = (box.y + box.height - 0.01f) / TILE_SIZE;

                for (int y = topTile; y <= bottomTile; y++) {
                    for (int x = leftTile; x <= rightTile; x++) {
                        int tileVal = GetTileAt(x, y);
                        if (tileVal == 1 || tileVal == 2) return true; 
                    }
                }
                return false;
            };

            // Process X and Y independently so sliding along walls feels smooth
            Rectangle nextXBox = playerHitBox; nextXBox.x += moveX;
            
            if (!isSolidArea(nextXBox)) {
                playerWorldPos.x += moveX;
                playerHitBox.x += moveX; // Update local hitbox for the Y check
            } else {
                // THE FIX: Snap flush to the wall on the X axis
                if (moveX > 0) { 
                    // Moving Right: Snap right side of hitbox to left side of wall
                    int tileX = (nextXBox.x + nextXBox.width) / TILE_SIZE;
                    float wallX = tileX * TILE_SIZE;
                    playerHitBox.x = wallX - nextXBox.width - 0.01f;
                } else if (moveX < 0) { 
                    // Moving Left: Snap left side of hitbox to right side of wall
                    int tileX = nextXBox.x / TILE_SIZE;
                    float wallX = (tileX + 1) * TILE_SIZE;
                    playerHitBox.x = wallX + 0.01f;
                }
                // Reverse the math to lock the world position to the snapped hitbox
                playerWorldPos.x = playerHitBox.x - ((standardWidth / 2.0f) - (hitBoxW / 2.0f));
            }

            Rectangle nextYBox = playerHitBox; nextYBox.y += moveY;
            
            if (!isSolidArea(nextYBox)) {
                playerWorldPos.y += moveY;
            } else {
                // THE FIX: Snap flush to the wall on the Y axis
                if (moveY > 0) { 
                    // Moving Down: Snap bottom of hitbox to top of wall
                    int tileY = (nextYBox.y + nextYBox.height) / TILE_SIZE;
                    float wallY = tileY * TILE_SIZE;
                    playerHitBox.y = wallY - nextYBox.height - 0.01f;
                } else if (moveY < 0) { 
                    // Moving Up: Snap top of hitbox to bottom of wall
                    int tileY = nextYBox.y / TILE_SIZE;
                    float wallY = (tileY + 1) * TILE_SIZE;
                    playerHitBox.y = wallY + 0.01f;
                }
                // Reverse the math to lock the world position to the snapped hitbox
                playerWorldPos.y = playerHitBox.y - (standardHeight - hitBoxH);
            }

            // --- MAP TRANSITION LOGIC ---
            // Use the center of our fixed footprint for transitions!
            int currentFootGridX = (playerHitBox.x + (hitBoxW / 2.0f)) / TILE_SIZE;
            int currentFootGridY = (playerHitBox.y + (hitBoxH / 2.0f)) / TILE_SIZE;
            int standingOnTile = GetTileAt(currentFootGridX, currentFootGridY);

            if (currentActiveMap == 0 && standingOnTile == 3) {
                currentActiveMap = 1;
                playerWorldPos = { 5.0f * TILE_SIZE, 5.0f * TILE_SIZE }; 
            } 
            else if (currentActiveMap == 1 && standingOnTile == 4) {
                currentActiveMap = 0;
                playerWorldPos = { 34.0f * TILE_SIZE, 7.0f * TILE_SIZE }; 
            }
            // --- LEAVING THE BAR ---
            else if (currentActiveMap == 0 && standingOnTile == 5) {
                currentActiveMap = 2;
                // Spawn above North door. Y=6 accounts for character height so feet hit Y=8.
                playerWorldPos = { 20.0f * TILE_SIZE, 6.0f * TILE_SIZE }; 
            }
            else if (currentActiveMap == 0 && standingOnTile == 7) {
                currentActiveMap = 2;
                // THE FIX: Changed X from 19.0f to 20.0f to align perfectly with the door!
                playerWorldPos = { 20.0f * TILE_SIZE, 17.0f * TILE_SIZE }; 
            }
            else if (currentActiveMap == 0 && standingOnTile == 8) {
                currentActiveMap = 2;
                // Spawn right of East door. Y=11 aligns feet with Y=13.
                playerWorldPos = { 35.0f * TILE_SIZE, 11.0f * TILE_SIZE }; 
            }
            // --- ENTERING THE BAR ---
            else if (currentActiveMap == 2 && standingOnTile == 6) {
                currentActiveMap = 0;
                // Spawn below inside North door. 
                playerWorldPos = { 10.0f * TILE_SIZE, 1.0f * TILE_SIZE }; 
            }
            else if (currentActiveMap == 2 && standingOnTile == 9) {
                currentActiveMap = 0;
                // THE FIX: Y=13 ensures feet land at Y=15, safely above the bottom wall!
                playerWorldPos = { 10.0f * TILE_SIZE, 13.0f * TILE_SIZE }; 
            }
            else if (currentActiveMap == 2 && standingOnTile == 10) {
                currentActiveMap = 0;
                // Y=5 aligns feet with Y=7.
                playerWorldPos = { 36.0f * TILE_SIZE, 5.0f * TILE_SIZE }; 
            }

            // --- CAMERA LOGIC ---
            exploreCamera.target = { playerWorldPos.x + (standardWidth / 2.0f), playerWorldPos.y + (standardHeight / 2.0f) };

            // Dynamic camera bounds for all 3 maps
            int activeMapWidth = (currentActiveMap == 0) ? 40 : (currentActiveMap == 1) ? 12 : 50;
            int activeMapHeight = (currentActiveMap == 0) ? 18 : (currentActiveMap == 1) ? 12 : 30;

            float minX = 0; float minY = 0;
            float maxX = activeMapWidth * TILE_SIZE; float maxY = activeMapHeight * TILE_SIZE;
            float viewWidth = 1920.0f / exploreCamera.zoom; float viewHeight = 1080.0f / exploreCamera.zoom;

            if ((maxX - minX) < viewWidth) {
                exploreCamera.target.x = minX + ((maxX - minX) / 2.0f);
            } else {
                if (exploreCamera.target.x - (viewWidth / 2.0f) < minX) exploreCamera.target.x = minX + (viewWidth / 2.0f);
                if (exploreCamera.target.x + (viewWidth / 2.0f) > maxX) exploreCamera.target.x = maxX - (viewWidth / 2.0f);
            }

            if ((maxY - minY) < viewHeight) {
                exploreCamera.target.y = minY + ((maxY - minY) / 2.0f);
            } else {
                if (exploreCamera.target.y - (viewHeight / 2.0f) < minY) exploreCamera.target.y = minY + (viewHeight / 2.0f);
                if (exploreCamera.target.y + (viewHeight / 2.0f) > maxY) exploreCamera.target.y = maxY - (viewHeight / 2.0f);
            }

            // --- DRAWING ---
            BeginDrawing(); ClearBackground(BLACK); 
            BeginMode2D(exploreCamera);

            // Draw the correct map dynamically!
            for (int y = 0; y < activeMapHeight; y++) {
                for (int x = 0; x < activeMapWidth; x++) {
                    Rectangle tileRec = { (float)(x * TILE_SIZE), (float)(y * TILE_SIZE), (float)TILE_SIZE, (float)TILE_SIZE };
                    int tileVal = GetTileAt(x, y);

                    if (tileVal == 1) { DrawRectangleRec(tileRec, DARKGRAY); DrawRectangleLinesEx(tileRec, 1, BLACK); }
                    else if (tileVal == 2) DrawRectangleRec(tileRec, BROWN); 
                    else if (tileVal == 3 || tileVal == 4) DrawRectangleRec(tileRec, DARKBLUE); 
                    else if (tileVal >= 5 && tileVal <= 10) DrawRectangleRec(tileRec, ORANGE); // All 6 doors are orange!
                    else {
                        // If it's a Floor (0), draw it differently based on the map
                        if (currentActiveMap == 2) DrawRectangleRec(tileRec, DARKGREEN); // Grass for Outside
                        else DrawRectangleRec(tileRec, LIGHTGRAY); // Wood/Tile for Inside
                    }
                }
            }

            float widthOffset = (standardWidth - pWidth) / 2.0f; // Centers thinner/wider sprites
            float heightOffset = pHeight - standardHeight;       // Anchors taller sprites to the floor

            Rectangle sourceRec = { 0.0f, 0.0f, (float)playerTex.width * playerFacing, (float)playerTex.height };
            
            Rectangle destRec = { playerWorldPos.x + widthOffset, playerWorldPos.y - heightOffset, pWidth, pHeight };
            
            DrawTexturePro(playerTex, sourceRec, destRec, {0,0}, 0.0f, WHITE);
            
            EndMode2D();
    
            DrawText("EXPLORATION MODE", 50, 50, 30, WHITE);
            DrawText("[ WASD to Move | SHIFT to Sprint | TAB for Menu | B for Debug Hub ]", 50, 90, 20, LIGHTGRAY);
            
            // Opens the new Quick Menu
            if (IsKeyReleased(KEY_TAB)) {
                currentScreen = MENU;
            }

            // Keeps your teleport back to the testing Hub
            if (IsKeyReleased(KEY_B)) {
                currentScreen = HUB;
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
            // Change this to return to MENU instead of HUB
            if (DrawMenuButton({100, 850, 300, 60}, "Back [B]", GRAY, false) || IsKeyReleased(KEY_B)) { 
                swapGroup = -1; 
                currentScreen = previousScreen; // <--- THE MAGIC FIX
            }
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

            // Change this to return to MENU instead of HUB
            if (DrawMenuButton({100, 850, 300, 60}, "Back [B]", GRAY, false) || IsKeyReleased(KEY_B)) { 
                swapGroup = -1; 
                currentScreen = previousScreen; // <--- THE MAGIC FIX
            }
            EndDrawing();
        }
        else if (currentScreen == CUSTOMIZE && selectedCharForCustomization != nullptr) {
            BeginDrawing(); ClearBackground(Fade(BLACK, 0.95f));
            DrawText(TextFormat("CUSTOMIZING: %s", selectedCharForCustomization->name.c_str()), 100, 30, 50, PURPLE);

            // 1. Profile Photo (Top Left next to name)
            if (globalSprites.count(selectedCharForCustomization->actorID)) {
                Texture2D tex = globalSprites[selectedCharForCustomization->actorID];
                
                int textWidth = MeasureText(TextFormat("CUSTOMIZING: %s", selectedCharForCustomization->name.c_str()), 50);
                Rectangle profileDest = { (float)(100 + textWidth + 20), 20, 60, 60 };
                
                // ---> THE FIX: Dynamic square crop <---
                Rectangle sourceCrop = {0, 0, (float)tex.width, (float)tex.width}; 
                
                DrawTexturePro(tex, sourceCrop, profileDest, {0,0}, 0.0f, WHITE);
                DrawRectangleLinesEx(profileDest, 2, WHITE);
            }

            // 2. Full Body Picture, Backstory & Natural Ability (Bottom Middle/Right)
            if (globalSprites.count(selectedCharForCustomization->actorID)) {
                Texture2D tex = globalSprites[selectedCharForCustomization->actorID];
                
                Rectangle boxDest = { 930, 550, 180, 340 }; 
                DrawRectangleRec(boxDest, DARKBLUE);
                DrawRectangleLinesEx(boxDest, 2, WHITE);
                
                Rectangle fullSource = {0, 0, (float)tex.width, (float)tex.height}; 
                
                // ---> THE FIX: Floor Anchoring Math <---
                float pixelScale = 10.0f; 
                
                float scaledWidth = tex.width * pixelScale;
                float scaledHeight = tex.height * pixelScale;
                
                // Anchor the floor to just above the bottom line of the blue box
                float floorY = boxDest.y + boxDest.height - 10.0f;
                
                // Center it horizontally in the box, and anchor the bottom to the floor
                Rectangle fullDest = { 
                    boxDest.x + (boxDest.width / 2.0f) - (scaledWidth / 2.0f), 
                    floorY - scaledHeight, 
                    scaledWidth, 
                    scaledHeight 
                };
                
                DrawTexturePro(tex, fullSource, fullDest, {0,0}, 0.0f, WHITE);

                std::string backstory = "A determined fighter trying to make a name in the streets.";
                if (selectedCharForCustomization->actorID == ActorID::Brian) backstory = "A natural talent who relies heavily on his sharp instincts.";
                else if (selectedCharForCustomization->actorID == ActorID::Paul) backstory = "A fierce brawler with an explosive temper and heavy hits.";
                else if (selectedCharForCustomization->actorID == ActorID::Vince) backstory = "The unwavering rock of the crew. Tough and unyielding.";
                else if (selectedCharForCustomization->actorID == ActorID::Joe) backstory = "YoOOOooo.";
                else if (selectedCharForCustomization->actorID == ActorID::Justin) backstory = "Your Favorite country white boy.";
                
                // Shifted backstory up
                DrawText("CHARACTER BACKGROUND:", 1120, 670, 25, GOLD);
                DrawText(backstory.c_str(), 1120, 710, 20, LIGHTGRAY);
                
                // NEW: Natural Ability Text rendered directly below the backstory
                if (selectedCharForCustomization->naturalAbility != NaturalID::None) {
                    DrawText("NATURAL ABILITY:", 1120, 810, 25, GREEN);
                    DrawText(getNaturalName(selectedCharForCustomization->naturalAbility).c_str(), 1120, 850, 22, GOLD);
                    
                    // Word wrap helper to keep the description inside the boundary
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
                    
                    std::string natDesc = wrapText(getNaturalDescription(selectedCharForCustomization->naturalAbility), 50);
                    DrawText(natDesc.c_str(), 1120, 880, 18, LIGHTGRAY);
                }
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
            
            // Read his actual stance to spoof the stats!
            if (selectedCharForCustomization->naturalAbility == NaturalID::ScrewDat && selectedCharForCustomization->isAltStance) {
                displayBIQ = selectedCharForCustomization->baseSIQ; 
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
                DrawText(selectedCharForCustomization->isAltStance ? "CURRENT MOVESET (STRIKE):" : "CURRENT MOVESET (SUPPORT):", 100, 150, 30, GREEN);
                                 
                if (DrawMenuButton({520, 145, 130, 40}, selectedCharForCustomization->isAltStance ? "-> SUPPORT" : "-> STRIKE", DARKGRAY, bgDisabled)) {
                    // This instantly changes his stats and swaps his arrays!
                    selectedCharForCustomization->toggleStance();
                }
            } else {
                DrawText("CURRENT MOVESET:", 100, 150, 30, GREEN);
            }
            int yOffset = 200;
            
            // Because toggleStance() swapped the arrays in the engine, 
            // we ALWAYS just edit the main combatMenu. No complex logic needed!
            std::vector<Move>& activeEditMenu = selectedCharForCustomization->combatMenu;

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
                        // ALWAYS edit the main combatMenu
                        std::vector<Move>& targetMenu = selectedCharForCustomization->combatMenu;
                        if (targetMenu.size() > 1) targetMenu.erase(targetMenu.begin() + customSlotIndex);
                        showCustomSelect = false; customScrollY = 0; 
                    }
                    gridY += 70; 

                    std::vector<MoveCategory> categories = { MoveCategory::Basic, MoveCategory::Skill, MoveCategory::Ultimate, MoveCategory::Support };
                    
                    for (MoveCategory cat : categories) {
                        DrawText(getCategoryName(cat).c_str(), 100, gridY, 30, SKYBLUE);
                        gridY += 40; gridX = 100;
                        
                        for (int i = 1; i < poolMoves.size(); i++) { 
                            MoveID mID = poolMoves[i];
                            const Move& m = getMove(mID);
                            
                            if (m.category == cat) {
                                Rectangle btn = { (float)gridX, (float)gridY, 350.0f, 50.0f };
                                
                                if (DrawMenuButton(btn, m.name.c_str(), DARKGRAY, listDisabled)) {
                                    // ALWAYS edit the main combatMenu
                                    std::vector<Move>& targetMenu = selectedCharForCustomization->combatMenu;
                                    targetMenu[customSlotIndex] = getMove(mID);
                                    showCustomSelect = false; customScrollY = 0; 
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
            if (showDetailsPopup) DrawDetailsOverlay(showDetailsPopup, detailsType, detailedMove, detailedTeamUpName, detailedPassive, detailedActive, detailedNatural, masterTeamUps);
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
            // ADDED THE NATURALS TAB
            if (DrawMenuButton({100, 480, 300, 60}, "5. Naturals", (activeTab == 4) ? GOLD : GRAY, bgDisabled)) { activeTab = 4; scrollY = 0; }
            // SHIFTED THIS TO #6
            // Replace your existing "6. Back to Hub [B]" line in ENCYCLOPEDIA with this:
            if (DrawMenuButton({100, 900, 300, 60}, "6. Back [B]", DARKGRAY, bgDisabled) || (!bgDisabled && IsKeyReleased(KEY_B))) { 
                currentScreen = previousScreen; 
                scrollY = 0; 
            }

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
            // ADD THIS ELSE IF BLOCK:
            else if (activeTab == 4) {
                totalContentHeight = 60 + (MASTER_NATURAL_POOL.size() - 1) * 80;
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
                        const Move& m = getMove(poolMoves[i]);
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
            } // <--- This closes the `else if (activeTab == 3)` block
            else if (activeTab == 4) {
                int yPos = 180 + scrollY;
                for (int i = 1; i < MASTER_NATURAL_POOL.size(); i++) { 
                    NaturalID nID = MASTER_NATURAL_POOL[i];
                    DrawText(getNaturalName(nID).c_str(), 480, yPos, 25, GREEN); 
                    DrawText(getNaturalDescription(nID).c_str(), 480, yPos + 30, 20, LIGHTGRAY); yPos += 80; 
                }
            }
            EndScissorMode(); // Stop clipping

            
            EndScissorMode(); // Stop clipping
            
            if (showDetailsPopup) DrawDetailsOverlay(showDetailsPopup, detailsType, detailedMove, detailedTeamUpName, detailedPassive, detailedActive, detailedNatural, masterTeamUps);
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
            
            reserves.resize(3, nullptr);
            
            // ---> NEW: PLUG THE MEMORY LEAK! <---
            // Delete the enemies from RAM, then clear the pointer list.
            for (Entity* enemy : enemyTeam) {
                if (enemy != nullptr) {
                    delete enemy;
                }
            }
            enemyTeam.clear();
            // ------------------------------------

            currentScreen = HUB;
        }
    }
    UnloadTexture(texBrian);
    UnloadTexture(texPaulIdle);
    UnloadTexture(texPaulPunch);
    UnloadTexture(texVince);
    UnloadTexture(texJoe);
    UnloadTexture(texJustin);

    UnloadTexture(texTony);

    UnloadTexture(texBully);
    UnloadTexture(texThug);
    CloseWindow(); return 0;
}