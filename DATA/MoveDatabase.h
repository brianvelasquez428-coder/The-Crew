#pragma once
#include "Entity.h"
#include <string>

Move getMove(MoveID moveID);
std::string getMoveName(MoveID moveID);
std::string getCategoryName(MoveCategory category);