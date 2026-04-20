#pragma once
#include "game_state.hpp"

class GameRules {
 public:
  // Returns true if side can legally move to dst on state.
  static bool isLegalMove(const GameState& state, Side side, Coord dst);
  // Returns true if tile can be legally broken on state.
  static bool isLegalBreak(const GameState& state, Coord tile);

  // Applies side's move to dst on state.
  static void applyMove(GameState& state, Side side, Coord dst);
  // Applies a tile break to state.
  static void applyBreak(GameState& state, Coord tile);

  // Returns true if side has at least one legal move on state.
  static bool hasAnyLegalMove(const GameState& state, Side side);
  // Returns true if the current state is terminal.
  static bool isTerminal(const GameState& state);
};
