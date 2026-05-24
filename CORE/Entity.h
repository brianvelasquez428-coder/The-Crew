#pragma once
#include <string>
#include <vector>

enum class LifeStage { Young, Teen, Adult };

enum class MoveTarget { Self, OneEnemy, TwoEnemies, AllEnemies, OneAlly, TwoAllies, AllAllies };

enum class Effect {
    None, RestoreStamina, DefenseScalingDamage, ApplyBleed, IgnoreDefense,
    DefenseBuff40, HealAndCleanse, LowerPriority, StrikeDefenseDebuff, StrikeUlt,
    SlowEnemy, HoldLineShield, PrecisionStrikeDebuff, StaminaStrip, P2BasicDebuff,
    ClutchGrab, LightsOutStun
};

// --- THE NEW ROSTER IDENTIFIERS ---
// EntityID handles backend logic like level-up stats and specific boss phases
enum class EntityID { 
    YoungBrian, YoungPaul, YoungVince, 
    AdultBrian, AdultPaul, AdultVince, 
    BossTony, StreetThug, ScrawnyThug, Unknown 
};

// ActorID handles generic identification for features like Team-Up requirements
enum class ActorID { 
    Brian, Paul, Vince, Tony, Enemy 
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
    UnstoppableATK, UnstoppableSPD, LightFeet, HeavyHands, IronWill 
};

enum class PassiveID { 
    None, ScrewDat, CrashOut, Mediator, Sparring, UnstoppableAssault, 
    LightOnTheFeet, HeavyHands, IronWill, SlippingPunches, Combo 
};

enum class ActiveID { 
    None, Adrenaline, Taunt, BodyWork, SlippingPunches, TheDeepEnd, 
    AnalyzeWeakness, RecklessAbandon, RealityCheck 
};

enum class StatusType { StatModifier, Bleed, Stun, Taunt, StaminaPenalty, StunImmune };
enum class StatName { None, ATK, DEF, SPD, BIQ, SIQ };

struct StatusEffect {
    StatusID id;
    StatusType type;
    StatName targetStat;
    int value;               
    int duration;            
    bool isStackable;    
};

std::string getStatusName(StatusID id);

enum class MoveID {
    None, Strike, TakeCover, SupportStrike, ClumsySwing,
    PaulBasic, OneTwoPunch, BobAndWeave, KnifeSlash, Haymaker,
    BrianStrikeBasic, BrianStrikeSkill, BrianStrikeUlt,
    BrianSupportBasic, BrianSupportSkill, BrianSupportUlt,
    VinceBasic, Shove, MiniSledge, Sledgehammer, HoldTheLine,
    TonyP1Basic, PrecisionStrike, OlderBrother,
    TonyP2Basic, Clutch, LightsOut,
    AllOutAttack, BrothersInArms, HeavyHitters,
    Unknown
};

struct Move {
    MoveID id;
    std::string name;
    int staminaCost;
    int hitCount;
    MoveCategory category; // <--- Swapped from string to Enum
    MoveTarget target;
    Effect effect;
    float powerMultiplier;
};

struct PhaseData {
    int thresholdHP; std::string transitionText;
    int newAttack; int newDefense; int newSpeed; int newBIQ; int newSIQ;
    PassiveID newNaturalAbility;
    std::vector<PassiveID> newPassiveAbilities; 
    std::vector<ActiveID> newActiveAbilities;
    std::vector<Move> newCombatMenu;
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
    
    PassiveID naturalAbility; bool usedActiveNatural;          
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
    int screwDatStacks; int screwDatDecayTimer;          
    int tempDamageBonus;                  
    int totalDamageTaken = 0; int totalDodges = 0;     
    bool hasScrapedKneesBadge = false; bool hasStreetSmartBadge = false;     
    
    std::vector<StatusEffect> activeStatuses;
    void addStatus(StatusEffect s);
    bool hasStatus(StatusType type);
    bool hasStatusID(StatusID id);
    void removeStatusByType(StatusType type);
    
    Entity(std::string spawnName, LifeStage spawnStage, bool spawnIsBoss); 
    void takeDamage(int rawDamage, bool isCritical);         
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
    void checkPhaseTransition();                             
    void resetStats();                                   
};