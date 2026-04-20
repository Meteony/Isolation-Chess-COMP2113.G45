#pragma once
#include "core/coord.hpp"
#include "core/game_state.hpp"

class Player {
 public:
  virtual ~Player() = default;

  // Prepares this player for the move phase using state.
  virtual void beginMovePhase(const GameState& state) = 0;
  // Prepares this player for the break phase using state.
  virtual void beginBreakPhase(const GameState& state) = 0;

  // Processes one input/update step using ch and state.
  virtual void update(int ch, const GameState& state) = 0;

  // Returns true when a move is ready to consume.
  virtual bool hasMoveReady() const = 0;
  // Returns the prepared move and clears that ready state.
  virtual Coord consumeMove() = 0;

  // Returns true when a break is ready to consume.
  virtual bool hasBreakReady() const = 0;
  // Returns the prepared break and clears that ready state.
  virtual Coord consumeBreak() = 0;
};
