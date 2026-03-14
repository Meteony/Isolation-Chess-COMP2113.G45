#pragma once
#include "coord.hpp"
#include "enums.hpp"

struct TurnRecord {
    Side actor;
    Coord moveCoord;
    Coord breakCoord;
    int thinkTicksBeforeMove = 0;
    int thinkTicksBeforeBreak = 0;
};
