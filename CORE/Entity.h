#pragma once
#include <string>
#include <vector>

enum class LifeStage { Young, Teen, Adult };

enum class MoveTarget { Self, OneEnemy, TwoEnemies, AllEnemies, OneAlly, TwoAllies, AllAllies };

enum class Effect {
    None, RestoreStamina, DefenseScalingDamage, ApplyBleed, IgnoreDefense,
    DefenseBuff40, HealAndCleanse, LowerPriority, StrikeDefenseDebuff, StrikeUlt,
    SlowEnemy, HoldLineShield, PrecisionStrikeDebuff, StaminaStrip, P2BasicDebuff,
    ClutchGrab, LightsOutStun, SelfAttackBuff10
};

// ---> NEW: The 3 Timing Windows <---
enum class EffectTiming { OnCast, PreHit, PostHit };

// --- THE NEW ROSTER IDENTIFIERS ---
// EntityID handles backend logic like level-up stats and specific boss phases
enum class EntityID { 
    YoungBrian, YoungPaul, YoungVince, 
    YoungJoe, YoungJustin,
    AdultBrian, AdultPaul, AdultVince, 
    BossTony, StreetThug, ScrawnyThug, Unknown 
};

// ActorID handles generic identification for features like Team-Up requirements
enum class ActorID { 
    Brian, Paul, Vince, Joe, Justin, Tony, Enemy 
};

// --- THE NEW MOVE CATEGORY ENUM ---
enum class MoveCategory { 
    Basic, Skill, Ultimate, Support, TeamUp 
};

enum class StatusID { 
    None, TauntedATK, TauntedDEF, Taunting, DeepEndSIQ, DeepEndBIQ, 
    Bleed, GuardBreak, Hardened, Slowed, GuardBroken, BoggedDown, 
    PrecisionDEFDrop, PrecisionBIQDrop, WindKnockedOut, Stunned, 
    DemolitionistATK, DemolitionistBIQ, CrashOutATK, CrashOutDEF, 
    UnstoppableATK, UnstoppableSPD, LightFeet, HeavyHands, IronWill,
    CrashOutHitBuffATK, CrashOutHitBuffSPD, CrashOutExhaustion, StunImmunity,
    SparringDEF, BasicAttackBuff // <--- ADD THIS HERE
};

enum class NaturalID { 
    None, ScrewDat, CrashOut, Mediator, Sparring, UnstoppableAssault 
};

enum class PassiveID { 
    None, LightOnTheFeet, HeavyHands, IronWill, SlippingPunches, Combo 
};

enum class ActiveID { 
    None, Adrenaline, Taunt, BodyWork, SlippingPunches, TheDeepEnd, 
    AnalyzeWeakness, RecklessAbandon, RealityCheck 
};

// We replace StatusType with StatusCategory for broader UI formatting
enum class StatusCategory { Buff, Debuff, DoT, HardCC, Special };
enum class StatName { None, ATK, DEF, SPD, BIQ, SIQ };

// The new Data-Driven Payload
struct StatusEffect {
    StatusID id;
    std::string name;           
    StatusCategory category;
    
    StatName targetStat;        
    int statModifier;           
    
    int dotDamage;              
    bool causesStun;            
    bool causesTaunt;           
    bool halvesStaminaRegen;    
    
    int duration;               
    bool isStackable;
};

std::string getStatusName(StatusID id);

enum class MoveID {
    // Universal
    None, Guard,
    // Enemies
    ClumsySwing, Strike,
    // Brian Young 
    StepOff, FaintPunches,
    // Brian
    BrianStrikeBasic, BrianStrikeSkill, BrianStrikeUlt,
    BrianSupportBasic, BrianSupportSkill, BrianSupportUlt,
    // Paul
    PaulBasic, OneTwoPunch, BobAndWeave, KnifeSlash, Haymaker,
    // Vince
    VinceBasic, Shove, MiniSledge, Sledgehammer, HoldTheLine,
    // Young Joe
    Windmill,
    // Young Justin
    Tackle,
    // Tony
    TonyP1Basic, PrecisionStrike, OlderBrother,
    TonyP2Basic, Clutch, LightsOut,

    SupportStrike,
    AllOutAttack, BrothersInArms, HeavyHitters,
    Unknown
};

struct Move {
    MoveID id;
    std::string name;
    int staminaCost;
    int hitCount;
    float powerMultiplier;
    MoveTarget target;
    Effect effect;
    
    EffectTiming effectTiming; // <--- NEW: Add this right below Effect!
    
    MoveCategory category;
    bool ignoresEvasion;
};

struct PhaseData {
    int thresholdHP; std::string transitionText;
    int newAttack; int newDefense; int newSpeed; int newBIQ; int newSIQ;
    NaturalID newNaturalAbility;
    std::vector<PassiveID> newPassiveAbilities; 
    std::vector<ActiveID> newActiveAbilities;
    std::vector<Move> newCombatMenu;
};

struct StatusPopup {
    std::string text;
    bool isBuff;
};

class Entity {
public:
    std::string name; 
    EntityID internalID;   // <--- Swapped to Enum
    ActorID actorID;       // <--- Added for TeamUps
    LifeStage currentStage; int currentPhase; bool isBoss;
    
    int maxHP; int currentHP;
    int baseAttack; int currentAttack;
    int baseDefense; int currentDefense;
    int baseSpeed; int currentSpeed;
    int baseBIQ; int currentBIQ;
    int baseSIQ; int currentSIQ;
    std::vector<PhaseData> extraPhases;      
    
    int maxStamina; int currentStamina; int staminaRegen;
    
    NaturalID naturalAbility; bool naturalActiveUsedThisBattle;  
    bool hasCrashedOut; 
    bool mediatorAwakened;      
    PassiveID hiddenAbility;  bool usedActiveAbilityThisTurn;           
    std::vector<PassiveID> passiveAbilities;       
    std::vector<ActiveID> activeAbilities;       
    std::vector<ActiveID> usedActives;      
    
    std::vector<Move> combatMenu;                    
    int level; int currentEXP; int expToNextLevel;
    int expDropValue; int moneyDropValue;      
    bool isAltStance;                                
    std::vector<Move> altCombatMenu;                 
    
    void toggleStance();                             
    
    // ---> FIX: Replaced std::string with Entity*
    bool isAlive; bool isGrabbed; Entity* grabbedBy;       
    int shieldHP; bool isCriticalOnlyShield;
    int hitNullificationStacks;
    
    int screwDatStacks;        // Tracks the decaying Strike Ultimate power
    int supportHealStacks;     // NEW: Tracks the permanent Support heal
    int screwDatDecayTimer;
    
    int tempDamageBonus;                  
    int totalDamageTaken = 0; int totalDodges = 0;     
    bool hasScrapedKneesBadge = false; bool hasStreetSmartBadge = false;     
    
    std::vector<StatusEffect> activeStatuses;
    void addStatus(StatusEffect s);
    bool hasStatus(StatusCategory category);
    bool hasStatusID(StatusID id);
    void removeStatusByType(StatusCategory category);
    
    Entity(std::string spawnName, LifeStage spawnStage, bool spawnIsBoss); 
    int takeDamage(int rawDamage, bool isCritical);         
    // --- NEW: EVENT HOOKS ---
    int getDamageReduction();
    void triggerOnHitPassives();
    // ------------------------       
    void healHP(int amount);                                 
    void endOfTurnUpdate();                                  
    void regenerateStamina();                                
    void gainEXP(int expAmount);       
    void levelUp();                    
    bool useStamina (int cost);                              
    bool executeTeamUp(Entity& partner, int staminaCost);                           
    bool executeTeamUp(Entity& partner1, Entity& partner2, int staminaCost);        
    void calculateActiveStats();                             
    void checkBadges();     
    std::vector<StatusPopup> pendingPopups;      // <--- ALREADY THERE
    std::vector<std::string> pendingLogMessages; // <--- ADD THIS FOR COMBAT LOG DELAYS
    bool checkPhaseTransition();            
    void resetStats();                                                                      
};