#pragma once
#include "core/coord.hpp"
#include "core/game_state.hpp"

class Player {
 public:
  virtual ~Player() = default;

  virtual void beginMovePhase(const GameState& state) = 0;
  virtual void beginBreakPhase(const GameState& state) = 0;

  virtual void update(int ch, const GameState& state) = 0;

  virtual bool hasMoveReady() const = 0;
  virtual Coord consumeMove() = 0;

  virtual bool hasBreakReady() const = 0;
  virtual Coord consumeBreak() = 0;
};
