#pragma once
#include "core/coord.hpp"
#include "core/enums.hpp"

// UI-only overlay state. Keep it simple.
struct InputOverlay
{
    bool cursorVisible = false;
    Coord cursor{0, 0};

    TurnPhase phaseHint = TurnPhase::NewTurn;
};