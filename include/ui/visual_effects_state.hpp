#pragma once
#include <optional>
#include "core/coord.hpp"

struct VisualEffectsState
{
    std::optional<Coord> lastMoveFrom;
    std::optional<Coord> lastMoveTo;
    std::optional<Coord> lastBreak;

    int flashTicks = 0;
};