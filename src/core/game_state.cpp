#include "core/game_state.hpp"

#include <cassert>

// GameState initializer
GameState::GameState(int rows, int cols)
    : m_rows(rows),
      m_cols(cols),
      m_tiles(rows * cols, TileState::Intact),
      m_p1Pos{(rows - 1) / 2, 0},
      m_p2Pos{rows / 2, cols - 1},
      m_sideToMove(Side::Player1),
      m_phase(TurnPhase::Move),
      m_status(SessionStatus::Running),
      m_winner(Side::Player1) {}

// Match Phase Accessors
TurnPhase GameState::phase() const { return m_phase; }

void GameState::setPhase(TurnPhase phase) { m_phase = phase; }

// GameState Index Accrssors
int GameState::index(Coord c) const { return c.row * m_cols + c.col; }

TileState GameState::tileAt(Coord c) const { return m_tiles[index(c)]; }

void GameState::setTile(Coord c, TileState state) { m_tiles[index(c)] = state; }

int GameState::rows() const { return m_rows; }

int GameState::cols() const { return m_cols; }

bool GameState::inBounds(Coord here) const {
  int x = here.col;
  int y = here.row;
  if (x < 0 || x >= cols()) return false;
  if (y < 0 || y >= rows()) return false;
  return true;
}

Coord GameState::playerPos(Side side) const {
  switch (side) {
    case Side::Player1:
      return m_p1Pos;
    case Side::Player2:
      return m_p2Pos;
    default:
      assert(false && "Invalid Side in GameState::playerPos");
      return {-1, -1};
  }
}

Side GameState::sideToMove() const { return m_sideToMove; }

void GameState::setSideToMove(Side side) { m_sideToMove = side; }

void GameState::setPlayerPos(Side side, Coord pos) {
  switch (side) {
    case Side::Player1:
      m_p1Pos = pos;
      return;
    case Side::Player2:
      m_p2Pos = pos;
      return;
  }
}

SessionStatus GameState::status() const { return m_status; }

void GameState::setStatus(SessionStatus status) { m_status = status; }

Side GameState::winner() const { return m_winner; }

void GameState::setWinner(Side side) { m_winner = side; }
