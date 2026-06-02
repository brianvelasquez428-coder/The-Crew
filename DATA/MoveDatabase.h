#pragma once 
#include "Entity.h" 
#include <string> 

// Change this line to return a const reference!
const Move& getMove(MoveID moveID); 

std::string getMoveName(MoveID moveID);
std::string getCategoryName(MoveCategory category);

// ---> ADD THESE HERE <---
std::string getTargetText(MoveTarget t);
std::string getEffectText(Effect e);