#pragma once
#include "game_state.hpp"

class GameRules {
public:
    static bool isLegalMove(const GameState& state, Side side, Coord dst);
    static bool isLegalBreak(const GameState& state, Coord tile);

    static void applyMove(GameState& state, Side side, Coord dst);
    static void applyBreak(GameState& state, Coord tile);

    static bool hasAnyLegalMove(const GameState& state, Side side);
    static bool isTerminal(const GameState& state);
};
