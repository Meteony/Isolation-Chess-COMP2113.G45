#include "ui/board_renderer.hpp"

#include <ncurses.h>

#include <algorithm>

#include "ui/ui_colors.hpp"

namespace {
// 4x2 logical tile size on screen.
constexpr int kTileRows = 2;
constexpr int kTileCols = 4;

// One blank column between the frame and the board.
constexpr int kLeftPad = 1;

// Convert a logical board row to the screen row inside the frame.
int tileScreenRow(const Coord& winPos, const Coord& boardOffset,
                  Coord boardCoord) {
  return winPos.row + 1 + (boardCoord.row - boardOffset.row) * kTileRows;
}

// Convert a logical board column to the screen column inside the frame.
int tileScreenCol(const Coord& winPos, const Coord& boardOffset,
                  Coord boardCoord) {
  return winPos.col + 1 + kLeftPad +
         (boardCoord.col - boardOffset.col) * kTileCols;
}

// Return whether a full 4x2 tile fits inside the frame interior.
bool tileVisible(const Coord& winPos, const Coord& size, int row, int col) {
  return row >= winPos.row + 1 && col >= winPos.col + 1 &&
         row + (kTileRows - 1) <= winPos.row + size.row - 2 &&
         col + (kTileCols - 1) <= winPos.col + size.col - 2;
}

// Draw a 4x2 glyph block with optional color and bold styling.
void drawGlyphBlock(int row, int col, const char* top, const char* bottom,
                    int colorPair = 0, bool bold = false) {
  if (colorPair != 0) attron(COLOR_PAIR(colorPair));
  if (bold) attron(A_BOLD);

  mvaddstr(row, col, top);
  mvaddstr(row + 1, col, bottom);

  if (bold) attroff(A_BOLD);
  if (colorPair != 0) attroff(COLOR_PAIR(colorPair));
}

void drawTileAt(const GameState& state, const Coord& winPos,
                const Coord& boardOffset, const Coord& size, Coord boardCoord,
                int colorPair = 0, bool isCursor = false) {
  const int row = tileScreenRow(winPos, boardOffset, boardCoord);
  const int col = tileScreenCol(winPos, boardOffset, boardCoord);

  if (!tileVisible(winPos, size, row, col)) return;

  if (isCursor) {
    drawGlyphBlock(row, col, "████", "████", colorPair);
    return;
  }
  // Broken tiles always use the same base glyphs.
  // Only the cursor highlight recolors them.
  if (state.tileAt(boardCoord) == TileState::Broken) {
    if (colorPair == CP_CURSOR_HL) {
      drawGlyphBlock(row, col, "####", "####", colorPair);
    } else {
      drawGlyphBlock(row, col, "####", "####");
    }
    return;
  }

  // Intact tiles use a light checker contrast for readability.
  const bool odd = ((boardCoord.row + boardCoord.col) & 1) != 0;
  if (odd) {
    if (colorPair == 0) {
      drawGlyphBlock(row, col, "    ", "    ", A_DIM);
    } else {
      drawGlyphBlock(row, col, "░░░░", "░░░░", colorPair);
    }
  } else {
    drawGlyphBlock(row, col, "░░░░", "░░░░", colorPair);
  }
}

void drawPieceAt(const Coord& winPos, const Coord& boardOffset,
                 const Coord& size, Coord boardCoord, int colorPair) {
  const int row = tileScreenRow(winPos, boardOffset, boardCoord);
  const int col = tileScreenCol(winPos, boardOffset, boardCoord);

  if (!tileVisible(winPos, size, row, col)) return;

  drawGlyphBlock(row, col, "▛▀▀▜", "▙▄▄▟", colorPair, true);
}

/*Draw those 2x1 triangles to indicate map getting cut off*/
void drawOverflowMarkers(const Coord& winPos, const Coord& size,
                         const Coord& boardOffset, const GameState& state) {
  const int visibleRows = std::max(1, (size.row - 2) / kTileRows);
  const int visibleCols = std::max(1, (size.col - 2 - kLeftPad) / kTileCols);

  const bool hiddenUp = boardOffset.row > 0;
  const bool hiddenDown = boardOffset.row + visibleRows < state.rows();
  const bool hiddenLeft = boardOffset.col > 0;
  const bool hiddenRight = boardOffset.col + visibleCols < state.cols();

  const int midRow = winPos.row + size.row / 2 - 1;
  const int midCol = winPos.col + size.col / 2 - 1;

  // attron(COLOR_PAIR(CP_FRAME_FOCUSED) | A_BOLD);

  // Left: 1x2 marker
  if (hiddenLeft) {
    mvaddstr(midRow, winPos.col + 1, "◢");
    mvaddstr(midRow + 1, winPos.col + 1, "◥");
  }

  // Right: 1x2 marker
  if (hiddenRight) {
    mvaddstr(midRow, winPos.col + size.col - 3, "◣");
    mvaddstr(midRow + 1, winPos.col + size.col - 3, "◤");
  }

  // Up: 2x1 marker
  if (hiddenUp) {
    mvaddstr(winPos.row + 1, midCol, "◢");
    mvaddstr(winPos.row + 1, midCol + 1, "◣");
  }

  // Down: 2x1 marker
  if (hiddenDown) {
    mvaddstr(winPos.row + size.row - 2, midCol, "◥");
    mvaddstr(winPos.row + size.row - 2, midCol + 1, "◤");
  }

  // attroff(COLOR_PAIR(CP_FRAME_FOCUSED) | A_BOLD);
}

// Clamp board scrolling so the visible region stays within the board.
void clampOffset(Coord& boardOffset, const Coord& size,
                 const GameState& state) {
  const int visibleRows = std::max(1, (size.row - 2) / kTileRows);
  const int visibleCols = std::max(1, (size.col - 2 - kLeftPad) / kTileCols);

  boardOffset.row =
      std::clamp(boardOffset.row, 0, std::max(0, state.rows() - visibleRows));
  boardOffset.col =
      std::clamp(boardOffset.col, 0, std::max(0, state.cols() - visibleCols));
}
}  // namespace

void BoardRenderer::drawFrame(bool winFocused) {
  ensureUiColorsInitialized();

  const int top = m_winPos.row;
  const int left = m_winPos.col;
  const int bottom = m_winPos.row + m_size.row - 1;
  const int right = m_winPos.col + m_size.col - 1;

  for (int r = top; r <= bottom; ++r) {
    for (int c = left; c <= right; ++c) {
      mvaddch(r, c, ' ');
    }
  }

  const int pair = winFocused ? CP_FRAME_FOCUSED : CP_FRAME;
  if (!winFocused) attron(A_DIM);
  attron(COLOR_PAIR(pair));

  mvaddstr(top, left, "╭");
  mvaddstr(top, right, "╮");
  mvaddstr(bottom, left, "╰");
  mvaddstr(bottom, right, "╯");

  for (int c = left + 1; c < right; ++c) {
    mvaddstr(top, c, "─");
    mvaddstr(bottom, c, "─");
  }

  for (int r = top + 1; r < bottom; ++r) {
    mvaddstr(r, left, "│");
    mvaddstr(r, right, "│");
  }

  mvaddstr(top, left + 2, "Game");

  attroff(COLOR_PAIR(pair));
  if (!winFocused) attroff(A_DIM);
}

void BoardRenderer::drawBoard(const GameState& state) {
  for (int r = 0; r < state.rows(); ++r) {
    for (int c = 0; c < state.cols(); ++c) {
      drawTileAt(state, m_winPos, m_boardOffset, m_size, Coord{r, c});
    }
  }
}

void BoardRenderer::drawPieces(const GameState& state) {
  drawPieceAt(m_winPos, m_boardOffset, m_size, state.playerPos(Side::Player1),
              CP_P1);
  drawPieceAt(m_winPos, m_boardOffset, m_size, state.playerPos(Side::Player2),
              CP_P2);
}

// Placeholder for future match-only board overlays.
void BoardRenderer::overlayMiscElements(const MatchVisualState& visualState) {
  (void)visualState;
}

// Placeholder for future replay-only board overlays.
void BoardRenderer::overlayMiscElements(const ReplayVisualState& visualState) {
  (void)visualState;
}

void BoardRenderer::scrollBoard(int key) {
  switch (key) {
    case 'H':
    case KEY_LEFT:
      --m_boardOffset.col;
      break;
    case 'L':
    case KEY_RIGHT:
      ++m_boardOffset.col;
      break;
    case 'K':
    case KEY_UP:
      --m_boardOffset.row;
      break;
    case 'J':
    case KEY_DOWN:
      ++m_boardOffset.row;
      break;
    default:
      break;
  }
}

void BoardRenderer::resize(int rows, int cols) {
  m_size.row = std::max(3, rows);
  m_size.col = std::max(3, cols);
}

void BoardRenderer::moveTo(int rows, int cols) {
  m_winPos.row = rows;
  m_winPos.col = cols;
}

void BoardRenderer::render(int key, bool winFocused,
                           const MatchSession& session) {
  ensureUiColorsInitialized();

  const GameState& state = session.state();
  const MatchVisualState& visual = session.visualState();

  scrollBoard(key);
  clampOffset(m_boardOffset, m_size, state);

  drawFrame(winFocused);
  drawBoard(state);

  if (visual.cursorVisible && state.phase() == TurnPhase::Move) {
    const Coord center = state.playerPos(state.sideToMove());
    const int moveColor = (state.sideToMove() == Side::Player1) ? CP_P1 : CP_P2;

    for (int dr = -1; dr <= 1; ++dr) {
      for (int dc = -1; dc <= 1; ++dc) {
        const Coord here{center.row + dr, center.col + dc};
        if (!state.inBounds(here)) continue;
        drawTileAt(state, m_winPos, m_boardOffset, m_size, here, moveColor);
      }
    }
  }

  drawPieces(state);

  /*Cursor*/
  if (winFocused && session.gameTick() % 12 < 9) {
    if (visual.cursorVisible && state.inBounds(visual.cursor)) {
      drawTileAt(state, m_winPos, m_boardOffset, m_size, visual.cursor, 0,
                 true);
    }
  }

  drawOverflowMarkers(m_winPos, m_size, m_boardOffset, state);
  overlayMiscElements(visual);
}

void BoardRenderer::render(int key, bool winFocused,
                           const ReplaySession& session) {
  ensureUiColorsInitialized();

  const GameState& state = session.state();
  const ReplayVisualState& visual = session.visualState();

  scrollBoard(key);
  clampOffset(m_boardOffset, m_size, state);

  drawFrame(winFocused);
  drawBoard(state);
  drawPieces(state);
  drawOverflowMarkers(m_winPos, m_size, m_boardOffset, state);
  overlayMiscElements(visual);
}
