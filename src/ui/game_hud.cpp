#include "ui/game_hud.hpp"

#include <ncurses.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "core/time.hpp"
#include "ui/ui_colors.hpp"

namespace {

struct ColoredWord {
  std::string text;
  int colorPair;  // 0 = default
  bool newline;   // explicit line break
};

int messageTagToPair(const std::string& tag) {
  if (tag == "BLACK") return CP_MSG_BLACK;
  if (tag == "RED") return CP_MSG_RED;
  if (tag == "GREEN") return CP_MSG_GREEN;
  if (tag == "YELLOW") return CP_MSG_YELLOW;
  if (tag == "BLUE") return CP_MSG_BLUE;
  if (tag == "MAGENTA") return CP_MSG_MAGENTA;
  if (tag == "CYAN") return CP_MSG_CYAN;
  if (tag == "WHITE") return CP_MSG_WHITE;
  return 0;
}

std::string toUpper(std::string s) {
  for (char& ch : s) {
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  }
  return s;
}

std::vector<ColoredWord> tokenizeMessage(const std::string& msg) {
  std::vector<ColoredWord> out;
  size_t i = 0;

  while (i < msg.size()) {
    if (msg[i] == '\n') {
      out.push_back({"", 0, true});
      ++i;
      continue;
    }

    while (i < msg.size() && std::isspace(static_cast<unsigned char>(msg[i])) &&
           msg[i] != '\n') {
      ++i;
    }

    if (i >= msg.size()) break;

    int pair = 0;

    if (msg[i] == '<') {
      size_t close = msg.find('>', i + 1);
      if (close != std::string::npos) {
        std::string tag = toUpper(msg.substr(i + 1, close - i - 1));
        int parsedPair = messageTagToPair(tag);
        if (parsedPair != 0) {
          pair = parsedPair;
          i = close + 1;

          while (i < msg.size() &&
                 std::isspace(static_cast<unsigned char>(msg[i])) &&
                 msg[i] != '\n') {
            ++i;
          }
        }
      }
    }

    if (i >= msg.size()) break;
    if (msg[i] == '\n') continue;

    size_t start = i;
    while (i < msg.size() &&
           !std::isspace(static_cast<unsigned char>(msg[i]))) {
      ++i;
    }

    if (start < i) {
      out.push_back({msg.substr(start, i - start), pair, false});
    }
  }

  return out;
}

std::vector<std::vector<ColoredWord>> wrapMessageWords(const std::string& msg,
                                                       int width) {
  std::vector<std::vector<ColoredWord>> rows;
  rows.push_back({});

  int currentWidth = 0;
  auto words = tokenizeMessage(msg);

  auto nextRow = [&]() {
    rows.push_back({});
    currentWidth = 0;
  };

  for (const auto& word : words) {
    if (word.newline) {
      nextRow();
      continue;
    }

    std::string remaining = word.text;

    while (!remaining.empty()) {
      if (rows.back().empty()) {
        int take = std::min(width, static_cast<int>(remaining.size()));
        rows.back().push_back(
            {remaining.substr(0, take), word.colorPair, false});
        currentWidth = take;
        remaining.erase(0, take);

        if (!remaining.empty()) {
          nextRow();
        }
      } else {
        int needed = 1 + static_cast<int>(remaining.size());
        if (currentWidth + needed <= width) {
          rows.back().push_back({remaining, word.colorPair, false});
          currentWidth += needed;
          remaining.clear();
        } else {
          nextRow();
        }
      }
    }
  }

  return rows;
}

void drawWordRow(int row, int col, const std::vector<ColoredWord>& words,
                 int width) {
  int x = 0;

  attrset(A_NORMAL);

  for (size_t i = 0; i < words.size() && x < width; ++i) {
    if (i > 0 && x < width) {
      mvaddch(row, col + x, ' ');
      ++x;
    }

    if (words[i].colorPair != 0) {
      attron(COLOR_PAIR(words[i].colorPair));
    } else {
      attrset(A_NORMAL);
    }

    int n = std::min(width - x, static_cast<int>(words[i].text.size()));
    mvaddnstr(row, col + x, words[i].text.c_str(), n);
    x += n;

    attrset(A_NORMAL);
  }

  while (x < width) {
    mvaddch(row, col + x, ' ');
    ++x;
  }
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
  if (!focused) attron(A_DIM);
  const int bottom = top + height - 1;
  const int right = left + width - 1;

  const int pair = focused ? CP_FRAME_FOCUSED : CP_FRAME;
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

  if (!title.empty()) {
    mvaddnstr(top, left + 2, title.c_str(), width - 2);
  }

  if (!focused) attroff(A_DIM);
  attroff(COLOR_PAIR(pair));
}

void drawScrollbar(int top, int col, int height, int visibleTop,
                   int totalLines) {
  // No scrolling needed.
  if (totalLines <= height) {
    for (int i = 0; i < height; ++i) {
      mvaddch(top + i, col, ' ');
    }
    return;
  }

  const int maxScroll = totalLines - height;

  // Classic proportional thumb size.
  const int thumbHeight = std::max(1, (height * height) / totalLines);
  const int thumbTravel = height - thumbHeight;

  const int thumbTop =
      top + (maxScroll == 0 ? 0 : (thumbTravel * visibleTop) / maxScroll);

  attron(COLOR_PAIR(CP_FRAME));
  attron(A_DIM);

  // Track
  for (int i = 0; i < height; ++i) {
    mvaddch(top + i, col, ' ');
  }

  attroff(A_DIM);

  // Thumb
  for (int i = 0; i < thumbHeight; ++i) {
    mvaddch(thumbTop + i, col, ACS_VLINE);
  }

  attroff(COLOR_PAIR(CP_FRAME));
}

std::string padOrTrimRight(const std::string& s, int width) {
  if (width <= 0) return "";
  if (static_cast<int>(s.size()) >= width) {
    return s.substr(0, width);
  }
  return s + std::string(width - s.size(), ' ');
}

std::string fitRightAligned(const std::string& s, int width) {
  if (width <= 0) return "";
  if (static_cast<int>(s.size()) <= width) return s;
  return s.substr(0, width);
}
}  // namespace

void GameHud::drawFrame(bool winFocused) {
  const int top = m_winPos.row;
  const int left = m_winPos.col;

  drawBoxFrame(top, left, m_hudHeight, m_size.col, "HUD", winFocused);
  drawBoxFrame(top + m_hudHeight, left, m_logHeight, m_size.col, "Log",
               winFocused);
  drawBoxFrame(top + m_hudHeight + m_logHeight, left, m_commandHeight,
               m_size.col, "", winFocused);
}

void GameHud::drawMessages(const std::vector<std::string>& msgs) {
  ensureUiColorsInitialized();

  const int innerWidth = m_size.col - 4;
  const int top = m_winPos.row + m_hudHeight;
  const int innerTop = top + 1;
  const int innerHeight = m_logHeight - 2;

  std::vector<std::vector<ColoredWord>> wrappedRows;
  for (const auto& msg : msgs) {
    auto rows = wrapMessageWords(msg, innerWidth);
    wrappedRows.insert(wrappedRows.end(), rows.begin(), rows.end());
  }

  const int totalLines = static_cast<int>(wrappedRows.size());
  const int maxScroll = std::max(0, totalLines - innerHeight);
  linesOfLogScrolled = std::clamp(linesOfLogScrolled, 0, maxScroll);

  const int start = std::max(0, totalLines - innerHeight - linesOfLogScrolled);

  for (int i = 0; i < innerHeight; ++i) {
    const int row = innerTop + i;
    const int idx = start + i;

    if (idx < totalLines) {
      drawWordRow(row, m_winPos.col + 2, wrappedRows[idx], innerWidth);
    } else {
      drawWordRow(row, m_winPos.col + 2, {}, innerWidth);
    }
  }
  const int scrollCol = m_winPos.col + m_size.col - 2;
  drawScrollbar(innerTop, scrollCol, innerHeight, start, totalLines);

  attrset(A_NORMAL);
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
  size_t viewStart = 0;

  if (m_commandCursorPos >= static_cast<size_t>(innerWidth)) {
    viewStart = m_commandCursorPos - static_cast<size_t>(innerWidth) + 1;
  }

  visible = m_command.substr(viewStart, static_cast<size_t>(innerWidth));
  visible = padOrTrimRight(visible, innerWidth);
  drawText(innerRow, innerLeft, visible);

  m_flashOn = ((m_tick / (int)(0.8 * kGameFps)) % 2 == 0);
  if (!m_flashOn) return;

  int cursorCol = static_cast<int>(m_commandCursorPos - viewStart);
  if (cursorCol >= 0 && cursorCol < innerWidth) {
    mvaddch(innerRow, innerLeft + cursorCol, ACS_BLOCK);
  }
}

void GameHud::drawHUD(const MatchSession& session) {
  const GameState& state = session.state();
  const int top = m_winPos.row + 1;
  const int left = m_winPos.col + 2;
  const int width = m_size.col - 4;

  drawText(top, left, "Turn:");

  const std::string rawTurnText = session.playerName(state.sideToMove());
  const std::string turnText = fitRightAligned(rawTurnText, width - 6);
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
  const int seconds = session.gameTick() / kGameFps;
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

  const std::string rawTurnText = session.playerName(state.sideToMove());
  const std::string turnText = fitRightAligned(rawTurnText, width - 6);
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
  m_size.row = std::max(minRows, rows - rows % 2);
  m_size.col = std::max(24, cols);
}

void GameHud::moveTo(int rows, int cols) {
  m_winPos.row = rows;
  m_winPos.col = cols;
}

void GameHud::render(int key, bool winFocused, const MatchSession& session) {
  ensureUiColorsInitialized();
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
  ensureUiColorsInitialized();

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
