#include "ui/game_hud.hpp"

#include <ncurses.h>

#include <algorithm>
#include <cctype>

namespace {
enum ColorPairId {
  CP_P1 = 1,
  CP_P2 = 2,
  CP_FRAME_FOCUSED = 3,
  CP_PHASE_MOVE = 4,
  CP_PHASE_BREAK = 5,
  CP_ACTIVE = 6
};

void ensureColorsInitialized() {
  static bool initialized = false;
  if (initialized) return;

  if (has_colors()) {
    start_color();
    use_default_colors();

    init_pair(CP_P1, COLOR_BLUE, -1);
    init_pair(CP_P2, COLOR_RED, -1);
    init_pair(CP_FRAME_FOCUSED, COLOR_GREEN, -1);
    init_pair(CP_PHASE_MOVE, COLOR_BLUE, -1);
    init_pair(CP_PHASE_BREAK, COLOR_RED, -1);
    init_pair(CP_ACTIVE, COLOR_GREEN, -1);
  }

  initialized = true;
}

void drawText(int row, int col, const std::string& text, int colorPair = 0,
              bool bold = false) {
  if (colorPair != 0) attron(COLOR_PAIR(colorPair));
  if (bold) attron(A_BOLD);

  mvaddnstr(row, col, text.c_str(), static_cast<int>(text.size()));

  if (bold) attroff(A_BOLD);
  if (colorPair != 0) attroff(COLOR_PAIR(colorPair));
}

void drawBoxFrame(int top, int left, int height, int width,
                  const std::string& title = "", bool focused = false) {
  const int bottom = top + height - 1;
  const int right = left + width - 1;

  if (focused) attron(COLOR_PAIR(CP_FRAME_FOCUSED));

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

  if (!title.empty()) {
    mvaddnstr(top, left + 1, title.c_str(), width - 2);
  }

  if (focused) attroff(COLOR_PAIR(CP_FRAME_FOCUSED));
}

std::string padOrTrimRight(const std::string& s, int width) {
  if (width <= 0) return "";
  if (static_cast<int>(s.size()) >= width) {
    return s.substr(0, width);
  }
  return s + std::string(width - s.size(), ' ');
}

std::vector<std::string> wrapMessage(const std::string& s, int width) {
  std::vector<std::string> out;
  if (width <= 0) return out;
  if (s.empty()) {
    out.push_back("");
    return out;
  }

  for (size_t i = 0; i < s.size(); i += static_cast<size_t>(width)) {
    out.push_back(s.substr(i, static_cast<size_t>(width)));
  }
  return out;
}
}  // namespace

void GameHud::drawFrame(bool winFocused) {
  const int top = m_winPos.row;
  const int left = m_winPos.col;

  drawBoxFrame(top, left, m_hudHeight, m_size.col, "─HUD", winFocused);
  drawBoxFrame(top + m_hudHeight, left, m_logHeight, m_size.col, "─Log",
               winFocused);
  drawBoxFrame(top + m_hudHeight + m_logHeight, left, m_commandHeight,
               m_size.col, "", winFocused);
}

void GameHud::drawMessages(const std::vector<std::string>& msgs) {
  const int innerWidth = m_size.col - 4;
  const int top = m_winPos.row + m_hudHeight;
  const int innerTop = top + 1;
  const int innerHeight = m_logHeight - 2;

  std::vector<std::string> wrapped;
  for (const auto& msg : msgs) {
    auto lines = wrapMessage(msg, innerWidth);
    wrapped.insert(wrapped.end(), lines.begin(), lines.end());
  }

  const int totalLines = static_cast<int>(wrapped.size());
  const int maxScroll = std::max(0, totalLines - innerHeight);
  linesOfLogScrolled = std::clamp(linesOfLogScrolled, 0, maxScroll);

  const int start = std::max(0, totalLines - innerHeight - linesOfLogScrolled);

  for (int i = 0; i < innerHeight; ++i) {
    const int row = innerTop + i;
    const int idx = start + i;

    if (idx < totalLines) {
      drawText(row, m_winPos.col + 2, padOrTrimRight(wrapped[idx], innerWidth));
    } else {
      drawText(row, m_winPos.col + 2, std::string(innerWidth, ' '));
    }
  }
}

void GameHud::drawCommand(bool winFocused) {
  const int top = m_winPos.row + m_hudHeight + m_logHeight;
  const int innerRow = top + 1;
  const int innerLeft = m_winPos.col + 2;
  const int innerWidth = m_size.col - 4;

  if (!winFocused) {
    drawText(innerRow, innerLeft,
             padOrTrimRight("Esc to use commands", innerWidth));
    return;
  }

  std::string visible = m_command;
  if (static_cast<int>(visible.size()) > innerWidth) {
    visible = visible.substr(visible.size() - static_cast<size_t>(innerWidth));
  }

  visible = padOrTrimRight(visible, innerWidth);
  drawText(innerRow, innerLeft, visible);

  m_flashOn = ((m_tick / 8) % 2 == 0);

  if (!m_flashOn) return;

  int cursorCol = static_cast<int>(m_commandCursorPos);
  if (cursorCol >= innerWidth) {
    cursorCol = innerWidth - 1;
  }

  mvaddch(innerRow, innerLeft + cursorCol, ACS_BLOCK);
}

void GameHud::drawHUD(const MatchSession& session) {
  const GameState& state = session.state();
  const int top = m_winPos.row + 1;
  const int left = m_winPos.col + 2;
  const int width = m_size.col - 4;

  drawText(top, left, "Turn:");

  const std::string turnText =
      (state.sideToMove() == Side::Player1) ? "Player 1" : "Player 2";
  const int turnColor = (state.sideToMove() == Side::Player1) ? CP_P1 : CP_P2;
  drawText(top, left + width - static_cast<int>(turnText.size()), turnText,
           turnColor);

  drawText(top + 1, left, "Phase:");

  std::string phaseText = "NewTurn";
  int phaseColor = 0;
  if (state.phase() == TurnPhase::Move) {
    phaseText = "   Move";
    phaseColor = CP_PHASE_MOVE;
  } else if (state.phase() == TurnPhase::Break) {
    phaseText = "  Break";
    phaseColor = CP_PHASE_BREAK;
  }

  drawText(top + 1, left + width - static_cast<int>(phaseText.size()),
           phaseText, phaseColor);

  /*Time: */
  const int seconds = session.gameTick() / 10;
  // const bool flashOn = ((session.gameTick() / 5) % 2 == 0);
  const std::string timeText = std::to_string(seconds) + " seconds";
  // const std::string emptyTimeText(timeText.size(), ' ');
  drawText(top + 2, left, "Time:");
  drawText(top + 2, left + width - static_cast<int>(timeText.size()), timeText);
}

void GameHud::drawHUD(const ReplaySession& session) {
  const GameState& state = session.state();
  const ReplayVisualState& vs = session.visualState();

  const int top = m_winPos.row + 1;
  const int left = m_winPos.col + 2;
  const int width = m_size.col - 4;

  drawText(top, left, "Turn:");

  const std::string turnText =
      (state.sideToMove() == Side::Player1) ? "Player 1" : "Player 2";
  const int turnColor = (state.sideToMove() == Side::Player1) ? CP_P1 : CP_P2;
  drawText(top, left + width - static_cast<int>(turnText.size()), turnText,
           turnColor);

  drawText(top + 1, left, "Phase:");

  std::string phaseText = "NewTurn";
  int phaseColor = 0;
  if (state.phase() == TurnPhase::Move) {
    phaseText = "   Move";
    phaseColor = CP_PHASE_MOVE;
  } else if (state.phase() == TurnPhase::Break) {
    phaseText = "  Break";
    phaseColor = CP_PHASE_BREAK;
  }

  drawText(top + 1, left + width - static_cast<int>(phaseText.size()),
           phaseText, phaseColor);

  drawText(top + 2, left, "Progress:");
  {
    const std::string progress =
        std::to_string(vs.currentTurn) + "/" + std::to_string(vs.totalTurn);
    drawText(top + 2, left + width - static_cast<int>(progress.size()),
             progress);
  }

  drawText(top + 3, left, "Playback:");
  {
    const std::string speed =
        std::to_string(vs.playbackSpeed).substr(0, 3) + "x";
    drawText(top + 3, left + width - static_cast<int>(speed.size()), speed);
  }

  drawText(top + 4, left, "Autoplay:");
  {
    const std::string autoText = vs.isAutoPlaying ? "Active" : "Off";
    drawText(top + 4, left + width - static_cast<int>(autoText.size()),
             autoText, vs.isAutoPlaying ? CP_ACTIVE : 0);
  }
}

void GameHud::resize(int rows, int cols) {
  const int minRows = 7 + 3 + 3;  // replay HUD + min log + command
  m_size.row = std::max(minRows, rows);
  m_size.col = std::max(24, cols);
}

void GameHud::moveTo(int rows, int cols) {
  m_winPos.row = rows;
  m_winPos.col = cols;
}

void GameHud::render(int key, bool winFocused, const MatchSession& session) {
  ensureColorsInitialized();
  ++m_tick;

  m_hudHeight = 5;
  m_commandHeight = 3;
  m_logHeight = std::max(3, m_size.row - m_hudHeight - m_commandHeight);

  if (winFocused) {
    if (key == KEY_UP) {
      ++linesOfLogScrolled;
    } else if (key == KEY_DOWN) {
      --linesOfLogScrolled;
    } else if (key == '\n' || key == KEY_ENTER || key == '\r') {
      m_commandReady = true;
    } else if (key == KEY_BACKSPACE || key == 127 || key == 8) {
      if (m_commandCursorPos > 0 && !m_command.empty()) {
        m_command.erase(m_commandCursorPos - 1, 1);
        --m_commandCursorPos;
      }
    } else if (key == KEY_LEFT) {
      if (m_commandCursorPos > 0) --m_commandCursorPos;
    } else if (key == KEY_RIGHT) {
      if (m_commandCursorPos < m_command.size()) ++m_commandCursorPos;
    } else if (key >= 32 && key <= 126) {
      m_command.insert(
          m_command.begin() + static_cast<std::ptrdiff_t>(m_commandCursorPos),
          static_cast<char>(key));
      ++m_commandCursorPos;
    }
  }

  drawFrame(winFocused);
  drawHUD(session);
  drawMessages(session.uiMessages());
  drawCommand(winFocused);
}

void GameHud::render(int key, bool winFocused, const ReplaySession& session) {
  ++m_tick;
  ensureColorsInitialized();

  m_hudHeight = 7;
  m_commandHeight = 3;
  m_logHeight = std::max(3, m_size.row - m_hudHeight - m_commandHeight);

  if (winFocused) {
    if (key == KEY_UP) {
      ++linesOfLogScrolled;
    } else if (key == KEY_DOWN) {
      --linesOfLogScrolled;
    } else if (key == '\n' || key == KEY_ENTER || key == '\r') {
      m_commandReady = true;
    } else if (key == KEY_BACKSPACE || key == 127 || key == 8) {
      if (m_commandCursorPos > 0 && !m_command.empty()) {
        m_command.erase(m_commandCursorPos - 1, 1);
        --m_commandCursorPos;
      }
    } else if (key == KEY_LEFT) {
      if (m_commandCursorPos > 0) --m_commandCursorPos;
    } else if (key == KEY_RIGHT) {
      if (m_commandCursorPos < m_command.size()) ++m_commandCursorPos;
    } else if (key >= 32 && key <= 126) {
      m_command.insert(
          m_command.begin() + static_cast<std::ptrdiff_t>(m_commandCursorPos),
          static_cast<char>(key));
      ++m_commandCursorPos;
    }
  }

  drawFrame(winFocused);
  drawHUD(session);
  drawMessages(session.uiMessages());
  drawCommand(winFocused);
}

std::optional<std::string> GameHud::consumeCommand() {
  if (!m_commandReady) {
    return std::nullopt;
  }

  std::optional<std::string> result = m_command;

  m_command.clear();
  m_commandCursorPos = 0;
  m_commandReady = false;

  return result;
}
