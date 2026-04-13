#include "ui/board_renderer.hpp"
#include <string>

const char *BoardRenderer::sideName(Side s) const
{
    switch (s)
    {
    case Side::Player1:
        return "Player1";
    case Side::Player2:
        return "Player2";
    }
    return "?";
}

const char *BoardRenderer::phaseName(TurnPhase p) const
{
    switch (p)
    {
    case TurnPhase::NewTurn:
        return "NewTurn";
    case TurnPhase::Move:
        return "Move";
    case TurnPhase::Break:
        return "Break";
    case TurnPhase::Finished:
        return "Finished";
    }
    return "?";
}

void BoardRenderer::draw(
    WINDOW *boardWin,
    WINDOW *hudWin,
    const MatchSession &session,
    const InputOverlay &overlay,
    const VisualEffectsState & /*vfx*/
) const
{
    werase(boardWin);
    werase(hudWin);

    box(boardWin, 0, 0);
    box(hudWin, 0, 0);

    mvwprintw(boardWin, 0, 2, " Board ");
    mvwprintw(hudWin, 0, 2, " HUD ");

    drawBoardWindow(boardWin, session.state(), overlay);
    drawHudWindow(hudWin, session);

    wrefresh(boardWin);
    wrefresh(hudWin);
}

void BoardRenderer::drawBoardWindow(WINDOW *boardWin, const GameState &state, const InputOverlay &overlay) const
{
    // Board top-left inside border
    int top = 1;
    int left = 2;

    for (int r = 0; r < state.rows(); ++r)
    {
        for (int c = 0; c < state.cols(); ++c)
        {
            Coord pos{r, c};

            char ch = (state.tileAt(pos) == TileState::Broken) ? '#' : '.';
            if (state.playerPos(Side::Player1) == pos)
                ch = '1';
            if (state.playerPos(Side::Player2) == pos)
                ch = '2';

            bool isCursor = overlay.cursorVisible && (overlay.cursor == pos);

            if (isCursor)
                wattron(boardWin, A_REVERSE);
            mvwaddch(boardWin, top + r, left + c * 2, ch);
            if (isCursor)
                wattroff(boardWin, A_REVERSE);
        }
    }
}

void BoardRenderer::drawHudWindow(WINDOW *hudWin, const MatchSession &session) const
{
    const GameState &state = session.state();

    int row = 1;
    int col = 2;

    mvwprintw(hudWin, row++, col, "Side to move: %s", sideName(state.sideToMove()));
    mvwprintw(hudWin, row++, col, "Phase:        %s", phaseName(state.phase()));
    mvwprintw(hudWin, row++, col, "Status:       %s",
              (state.status() == SessionStatus::Running) ? "Running" : "Finished");

    if (state.status() == SessionStatus::Finished)
    {
        mvwprintw(hudWin, row++, col, "Winner:       %s", sideName(state.winner()));
    }

    row++;
    mvwprintw(hudWin, row++, col, "Q: back to menu");
}