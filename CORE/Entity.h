// Contains the foundation for the build of every character
#pragma once
#include <string>
#include <vector>
// =============== All the enum classes used to avoid typos for string uses ===============

// Will be deleted, just used to distinguish multiple different versions of a character
enum class LifeStage { Young, Teen, Adult };

enum class MoveTarget { Self, OneEnemy, TwoEnemies, AllEnemies, OneAlly, TwoAllies, AllAllies };

enum class Effect {
    None, RestoreStamina, DefenseScalingDamage, ApplyBleed, IgnoreDefense,
    DefenseBuff40, HealAndCleanse, LowerPriority, StrikeDefenseDebuff, StrikeUlt,
    SlowEnemy, HoldLineShield, PrecisionStrikeDebuff, StaminaStrip, P2BasicDebuff,
    ClutchGrab, LightsOutStun, SelfAttackBuff10
};

// Determines if move effects should happen immediately without needing to connect, before it connects, or after it connects with a missed move of course not doing anything
enum class EffectTiming { OnCast, PreHit, PostHit };

// The background identity read by the computer for stats and phases that has every version separate
enum class EntityID { 
    YoungBrian, YoungPaul, YoungVince, 
    YoungJoe, YoungJustin,
    AdultBrian, AdultPaul, AdultVince, 
    BossTony, StreetThug, ScrawnyThug, Unknown 
};

// The frontend identity that the player sees that encompasses all versions into one
enum class ActorID { 
    Brian, Paul, Vince, Joe, Justin, Tony, ScrawnyThug, StreetThug, Enemy 
};

// Categorizes moves into certain types to vary in power and use
enum class MoveCategory { 
    Basic, Skill, Ultimate, Support, TeamUp 
};

// A list of ALL the effects a character can have
enum class StatusID { 
    None, TauntedATK, TauntedDEF, Taunting, DeepEndSIQ, DeepEndBIQ, 
    Bleed, GuardBreak, Hardened, Slowed, GuardBroken, BoggedDown, 
    PrecisionDEFDrop, PrecisionBIQDrop, WindKnockedOut, Stunned, 
    DemolitionistATK, DemolitionistBIQ, CrashOutATK, CrashOutDEF, 
    UnstoppableATK, UnstoppableSPD, LightFeet, HeavyHands, IronWill,
    CrashOutHitBuffATK, CrashOutHitBuffSPD, CrashOutExhaustion, StunImmunity,
    SparringDEF, BasicAttackBuff
};

// The natural ability that everybody has that's essentially their "perk" for using them
enum class NaturalID { 
    None, ScrewDat, CrashOut, Mediator, Sparring, UnstoppableAssault 
};

// A list of general abilities that can be added or removed from a character that trigger on their own either immediately or through a trigger (1 max)
enum class PassiveID { 
    None, LightOnTheFeet, HeavyHands, IronWill, SlippingPunches, Combo 
};

// A list of general abilities that can be added or removed from a character that can be activated by the player once the conditions are met if there are any (2-3 if have passive or not)
enum class ActiveID { 
    None, Adrenaline, Taunt, BodyWork, SlippingPunches, TheDeepEnd, 
    AnalyzeWeakness, RecklessAbandon, RealityCheck 
};

// All the effect types in the game
enum class StatusCategory { Buff, Debuff, DoT, HardCC, Special };

// Everything that you can put an effect on
enum class StatName { None, ATK, DEF, SPD, BIQ, SIQ };

// The foundation build for any given effect used
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

// All of the move names
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

// The foundation build for any given move used
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

// The foundation build for any given phase used
struct PhaseData {
    int thresholdHP; std::string transitionText;
    int newAttack; int newDefense; int newSpeed; int newBIQ; int newSIQ;
    NaturalID newNaturalAbility;
    std::vector<PassiveID> newPassiveAbilities; 
    std::vector<ActiveID> newActiveAbilities;
    std::vector<Move> newCombatMenu;
};

// Used for when effects show on screen to decide if its green or red
struct StatusPopup {
    std::string text;
    bool isBuff;
};

// The foundation build for any given character made
class Entity {
public:
    // Identity
    std::string name; 
    EntityID internalID;   // <--- Swapped to Enum
    ActorID actorID;       // <--- Added for TeamUps
    LifeStage currentStage; int currentPhase; bool isBoss;
    
    // Stats
    int maxHP; int currentHP;
    int baseAttack; int currentAttack;
    int baseDefense; int currentDefense;
    int baseSpeed; int currentSpeed;
    int baseBIQ; int currentBIQ;
    int baseSIQ; int currentSIQ;
    std::vector<PhaseData> extraPhases;      
    
    int maxStamina; int currentStamina; int staminaRegen;
    // Abilities
    NaturalID naturalAbility; bool naturalActiveUsedThisBattle;  
    bool hasCrashedOut; 
    bool mediatorAwakened;      
    PassiveID hiddenAbility;  bool usedActiveAbilityThisTurn;           
    std::vector<PassiveID> passiveAbilities;       
    std::vector<ActiveID> activeAbilities;       
    std::vector<ActiveID> usedActives;      
    
    // Menu, exp, and money
    std::vector<Move> combatMenu;                    
    int level; int currentEXP; int expToNextLevel;
    int expDropValue; int moneyDropValue;      
    bool isAltStance;                                
    std::vector<Move> altCombatMenu;   // holds a separate second moveset               
    
    void toggleStance();                             
    
    // Statuses or effects on a character
    bool isAlive; bool isGrabbed; Entity* grabbedBy;       
    int shieldHP; bool isCriticalOnlyShield;
    int hitNullificationStacks;
    
    // Brian specific
    int screwDatStacks;        // Tracks the decaying Strike Ultimate power
    int supportHealStacks;     // Tracks the permanent Support heal
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
    int getDamageReduction();
    void triggerOnHitPassives();      
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
    std::vector<StatusPopup> pendingPopups;    
    std::vector<std::string> pendingLogMessages; 
    bool checkPhaseTransition();            
    void resetStats();                                                                      
};