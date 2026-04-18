#pragma once
#include <vector>

#include "coord.hpp"
#include "enums.hpp"

/*
 Stores the complete board and turn state
 at a given instant.
*/

class GameState {
 private:
  int m_rows;
  int m_cols;
  std::vector<TileState> m_tiles;  // Board storage.

  Coord m_p1Pos;
  Coord m_p2Pos;

  Side m_sideToMove;       // Which player acts next.
  TurnPhase m_phase;       // Current phase within the turn.
  SessionStatus m_status;  // Ongoing / finished state of the session.
  Side m_winner;           // Set only when status is Finished.

 public:
  // Creates a board with the given dimensions.
  GameState(int rows, int cols);

  GameState(const GameState& other) = default;
  GameState& operator=(const GameState& other) = default;
  ~GameState() = default;

  int rows() const;
  int cols() const;

  // Returns true if c lies within the board dimensions.
  bool inBounds(Coord c) const;

  // Converts a board coordinate to the corresponding row-major array index.
  int index(Coord c) const;

  // Returns the tile at c.
  // (c must be in bounds)
  TileState tileAt(Coord c) const;

  // Sets the tile state at c.
  // (c must be in bounds)
  void setTile(Coord c, TileState state);

  // Returns the current position of the given player.
  Coord playerPos(Side side) const;

  // Updates the stored position of the given player.
  void setPlayerPos(Side side, Coord pos);

  Side sideToMove() const;
  void setSideToMove(Side side);

  TurnPhase phase() const;
  void setPhase(TurnPhase phase);

  SessionStatus status() const;
  void setStatus(SessionStatus status);

  // Returns the winner. Meaningful only if status() == Finished.
  Side winner() const;

  // Sets the winner of the session.
  void setWinner(Side side);
};
