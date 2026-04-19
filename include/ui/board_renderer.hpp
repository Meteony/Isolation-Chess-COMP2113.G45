#pragma once
#include <core/coord.hpp>
#include <core/game_state.hpp>
#include <sessions/match_session.hpp>
#include <sessions/replay_session.hpp>
#include <vector>

#include "core/enums.hpp"

/*
Fullly maximized state (doesn't go beyond this size):
╭─Game─────────────────────────────────────────╮
│ ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒ │
│ ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒ │
│     ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒     │
│     ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒     │
│ ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒ │
│ ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒ │
│     ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒     │
│     ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒     │
│ ▛▀▀▜    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒ │
│ ▙▄▄▟    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒ │
│     ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒     │
│     ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒     │
│ ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒ │
│ ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒ │
│     ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒     │
│     ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒     │
│ ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒ │
│ ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒    ▒▒▒▒ │
╰──────────────────────────────────────────────╯
*/

/*
Texture used:
▒▒▒▒    -> Intact
▒▒▒▒
        -> Broken

▛▀▀▜   -> Player piece
▙▄▄▟
*/

class BoardRenderer {
 private:
  Coord m_size;
  Coord m_winPos;
  Coord m_boardOffset;

  /*Size and pos should already be known*/
  void drawFrame(bool winFocused);

  /*Tiles only. Mode independent*/
  void drawBoard(const GameState& state);

  /*Pieces are drawn at this stage. Blue: P1, Red: P2*/
  /*Mode independent. GameState contains all piece info*/
  void drawPieces(const GameState& state);

  /*Overloaded for both modes*/
  /*Used to add misc visual elements that aren't pure tiles. */
  /*Cursor (Yellow), 3x3 highlighted (Green) region for move phase, etc*/
  void overlayMiscElements(const MatchVisualState& visualState);
  void overlayMiscElements(const ReplayVisualState& visualState);

  void scrollBoard(int key);

 public:
  BoardRenderer() : m_size{19, 48}, m_winPos{0, 0}, m_boardOffset{0, 0} {}

  /*Should be a direct interface to set an absolute size*/
  /*which will be externally computed & balanced between hud and this*/
  void resize(int rows, int cols);

  void moveTo(int rows, int cols);

  /*Overloaded to support both replay session and match session*/
  /*Takes a key as input to support scrolling behavior*/
  void render(int key, bool winFocused, /*Highlight boarder in green if true*/
              const MatchSession&);

  void render(int key, bool winFocused, const ReplaySession&);
  int bottomRow() const { return m_winPos.row + m_size.row - 1; }
  Coord size() const { return m_size; }
};
