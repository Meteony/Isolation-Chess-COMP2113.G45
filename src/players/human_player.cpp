#include "human_player.hpp"
#include <ncurses.h>

namespace {
    Coord clampedMove(Coord c, const GameState& state) {
        if (c.row < 0) c.row = 0;
        if (c.col < 0) c.col = 0;
        if (c.row >= state.rows()) c.row = state.rows() - 1;
        if (c.col >= state.cols()) c.col = state.cols() - 1;
        return c;
    }

    bool isConfirmKey(int ch) {
        return ch == '\n' || ch == 'c' || ch == 'C' || ch == KEY_ENTER || ch == '\r';
    }

    bool isCancelKey(int ch) {
        return ch == 27 || ch == 'x' || ch == 'X'; // ESC or x
    }
}

HumanPlayer::HumanPlayer()
    : m_cursor{},
      m_pendingMove{}, // Consumed by ConsumeMove
      m_pendingBreak{}, // Consumed by ConsumeBreak
      m_moveReady(false),
      m_breakReady(false)
{
}

void HumanPlayer::beginMovePhase(const GameState& state) {

    m_moveReady = false;
    m_breakReady = false;
    m_hasSelectedMove = false;

    // Start cursor at current player's actual position.
    m_cursor = state.playerPos(state.sideToMove());
    m_pendingMove = Coord{};
    m_pendingBreak = Coord{};
}

void HumanPlayer::beginBreakPhase(const GameState& state) {

    m_breakReady = false;

    // Keep selected move information for rendering if needed later.
    m_hasSelectedMove = true;

    // After MatchSession applies the move, state.playerPos(sideToMove())
    // should already be the moved-to tile. Start break selection there.
    m_cursor = state.playerPos(state.sideToMove());
}

void HumanPlayer::update(int ch, const GameState& state) {
    TurnPhase curPhase {state.phase()};

    if (ch == ERR) {
        return;
    }

    Coord next = m_cursor;

    switch (ch) {
    case KEY_UP:
    case 'w':
    case 'W':
        --next.row;
        break;

    case KEY_DOWN:
    case 's':
    case 'S':
        ++next.row;
        break;

    case KEY_LEFT:
    case 'a':
    case 'A':
        --next.col;
        break;

    case KEY_RIGHT:
    case 'd':
    case 'D':
        ++next.col;
        break;

    default:
        break;
    }

    // Apply cursor movement if the key was directional.
    if (next != m_cursor) {
        m_cursor = clampedMove(next, state);
        return;
    }

    // Confirm current cursor as the move/break choice.
    if (isConfirmKey(ch)) {
        if (curPhase == TurnPhase::Move) {
            m_pendingMove = m_cursor;
            m_moveReady = true;
        } else if (curPhase == TurnPhase::Break) {
            m_pendingBreak = m_cursor;
            m_breakReady = true;
        }
        return;
    }

    // Optional cancel behavior:
    // - in break phase, go back to moved tile
    // - in move phase, go back to current player position
    if (isCancelKey(ch)) {
        if (curPhase == TurnPhase::Move) {
            m_cursor = state.playerPos(state.sideToMove());
            m_moveReady = false;
        } else if (curPhase == TurnPhase::Break) {
            m_cursor = state.playerPos(state.sideToMove());
            m_breakReady = false;
        }
    }
}

bool HumanPlayer::hasMoveReady() const {
    return m_moveReady;
}

Coord HumanPlayer::consumeMove() {
    m_moveReady = false;
    return m_pendingMove;
}

bool HumanPlayer::hasBreakReady() const {
    return m_breakReady;
}

Coord HumanPlayer::consumeBreak() {
    m_breakReady = false;
    return m_pendingBreak;
}

const Coord HumanPlayer::cursor() const {
    return m_cursor;
}
