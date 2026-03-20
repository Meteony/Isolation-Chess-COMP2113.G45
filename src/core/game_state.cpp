#include "game_state.hpp"

// GameState initializer
GameState::GameState(int rows, int cols)
    : m_rows(rows),
      m_cols(cols),
      m_tiles(rows * cols, TileState::Intact),
      m_p1Pos{0, 0},
      m_p2Pos{rows - 1, cols - 1},
      m_sideToMove(Side::Player1),
      m_phase(TurnPhase::Move),
      m_status(SessionStatus::Running),
      m_winner(Side::Player1)
{
}

// Match Phase Accessors
TurnPhase GameState::phase() const {
    return m_phase;
}

void GameState::setPhase(TurnPhase phase) {
    m_phase = phase;
}

// GameState Index Accrssors
int GameState::index(Coord c) const {
    return c.row * m_cols + c.col;
}

TileState GameState::tileAt(Coord c) const {
    return m_tiles[index(c)];
}

void GameState::setTile(Coord c, TileState state) {
    m_tiles[index(c)] = state;
}
