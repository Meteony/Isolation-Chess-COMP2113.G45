#pragma once
#include <ncurses.h>

#include "sessions/match_session.hpp"
#include "ui/input_overlay.hpp"
#include "ui/visual_effects_state.hpp"

class BoardRenderer
{
public:
    void draw(
        WINDOW *boardWin,
        WINDOW *hudWin,
        const MatchSession &session,
        const InputOverlay &overlay,
        const VisualEffectsState &vfx) const;

private:
    void drawBoardWindow(WINDOW *boardWin, const GameState &state, const InputOverlay &overlay) const;
    void drawHudWindow(WINDOW *hudWin, const MatchSession &session) const;

    const char *sideName(Side s) const;
    const char *phaseName(TurnPhase p) const;
};