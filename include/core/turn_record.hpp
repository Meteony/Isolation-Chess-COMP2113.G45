#pragma once
#include "coord.hpp"
#include "enums.hpp"

struct TurnRecord {
  Side actor;
  Coord moveCoord;
  Coord breakCoord;
  long thinkTicksBeforeMove = 0;
  long thinkTicksBeforeBreak = 0;

  void reset(Side newActor) {
    actor = newActor;
    moveCoord = Coord{};
    breakCoord = Coord{};
    thinkTicksBeforeMove = 0;
    thinkTicksBeforeBreak = 0;
  }
};
