#pragma once

#include <ncurses.h>

#include <cctype>
#include <clocale>
#include <optional>
#include <string>
#include <vector>

#include "core/replay_io.hpp"
#include "sessions/match_session.hpp"

namespace scenes {

enum class FocusTarget { Game, Hud };

class NcursesGuard {
 public:
  NcursesGuard() {
    std::setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(0);
  }

  NcursesGuard(const NcursesGuard&) = delete;
  NcursesGuard& operator=(const NcursesGuard&) = delete;

  ~NcursesGuard() { endwin(); }
};

inline std::string trimCopy(std::string s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.erase(s.begin());
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.pop_back();
  }
  return s;
}

inline bool isValidReplayName(const std::string& name) {
  if (name.empty()) {
    return true;
  }
  if (name.size() > 32 || name == "." || name == "..") {
    return false;
  }
  for (char ch : name) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    if (!(std::isalnum(uch) || ch == '_' || ch == '-' || ch == '.')) {
      return false;
    }
  }
  return true;
}

inline std::string ellipsizeKeyTip(const std::string& s, int width) {
  if (width <= 0) {
    return "";
  }
  if (static_cast<int>(s.size()) <= width) {
    return s;
  }
  if (width <= 3) {
    return std::string(width, '.');
  }
  return s.substr(0, width - 3) + "...";
}

inline std::string buildBottomKeyTip(const std::vector<std::string>& items,
                                     int width) {
  if (width <= 0 || items.empty()) {
    return "";
  }

  std::string shown;
  std::size_t shownCount = 0;

  for (std::size_t i = 0; i < items.size(); ++i) {
    const std::string candidate =
        shown.empty() ? items[i] : shown + " • " + items[i];
    if (static_cast<int>(candidate.size()) > width) {
      break;
    }
    shown = candidate;
    shownCount = i + 1;
  }

  if (shownCount == items.size()) {
    return shown;
  }

  if (shown.empty()) {
    return ellipsizeKeyTip(items.front(), width);
  }

  std::string withEllipsis = shown + " • ...";
  while (static_cast<int>(withEllipsis.size()) > width) {
    const std::size_t split = shown.rfind(" • ");
    if (split == std::string::npos) {
      if (width >= 3) {
        return "...";
      }
      return std::string(width, '.');
    }
    shown.erase(split);
    withEllipsis = shown + " • ...";
  }

  return withEllipsis;
}

inline int chooseBottomKeyTipRow(int uiBottom) {
  int screenRows = 0;
  int screenCols = 0;
  getmaxyx(stdscr, screenRows, screenCols);

  if (screenRows <= 0 || screenCols <= 0) {
    return -1;
  }

  if (uiBottom + 1 < screenRows) {
    return uiBottom + 1;
  }

  return ((screenRows & 1) != 0) ? (screenRows - 1) : -1;
}

inline void drawBottomKeyTip(int uiBottom, int uiWidth,
                             const std::vector<std::string>& items) {
  int screenRows = 0;
  int screenCols = 0;
  getmaxyx(stdscr, screenRows, screenCols);

  const int row = chooseBottomKeyTipRow(uiBottom);
  if (row < 0 || screenCols <= 0) {
    return;
  }

  const bool appendedBelowUi = (row == uiBottom + 1);
  const int spanWidth =
      appendedBelowUi ? std::min(uiWidth, screenCols) : screenCols;
  if (spanWidth <= 0) {
    return;
  }

  const std::string text = buildBottomKeyTip(items, spanWidth);
  const int left =
      std::max(0, (spanWidth - static_cast<int>(text.size())) / 2);

  mvhline(row, 0, ' ', screenCols);
  attron(A_DIM);
  mvaddnstr(row, left, text.c_str(), spanWidth - left);
  attroff(A_DIM);
}

inline bool handleStandardSaveQuitCommand(MatchSession& session,
                                          const std::string& rawCommand,
                                          bool& running) {
  const std::string s = trimCopy(rawCommand);

  if (s == ":quit!") {
    running = false;
    return true;
  }

  if (s.compare(0, 6, ":quit!") == 0) {
    session.postUiMessage("<MAGENTA>[!] :quit! does not take an argument");
    return true;
  }

  if (s == ":h" || s == ":help") {
    session.postUiMessage("<YELLOW>[i] Commands:");
    session.postUiMessage("  :h / :help        Show this help");
    session.postUiMessage("  :save [name]      Save replay");
    session.postUiMessage("  :quit [name]      Save replay and quit");
    session.postUiMessage("  :quit!            Quit without saving");
    return true;
  }

  if (s.compare(0, 2, ":h") == 0 && s != ":h") {
    session.postUiMessage("<MAGENTA>[!] :h does not take an argument");
    return true;
  }

  if (s.compare(0, 5, ":help") == 0 && s != ":help") {
    session.postUiMessage("<MAGENTA>[!] :help does not take an argument");
    return true;
  }

  if (s == ":save" || (s.size() > 5 && s.compare(0, 5, ":save") == 0 &&
                       std::isspace(static_cast<unsigned char>(s[5])))) {
    const std::string name = (s.size() > 5) ? trimCopy(s.substr(5)) : "";

    if (!isValidReplayName(name)) {
      session.postUiMessage("<MAGENTA>[!] Invalid replay name");
      return true;
    }

    const ReplayData replay = session.buildReplayData();
    if (ReplayIO::saveReplay(replay, name)) {
      session.postUiMessage("<YELLOW>[i] Replay saved");
    } else {
      session.postUiMessage("<MAGENTA>[!] Failed to save replay");
    }
    return true;
  }

  if (s == ":quit" || (s.size() > 5 && s.compare(0, 5, ":quit") == 0 &&
                       std::isspace(static_cast<unsigned char>(s[5])))) {
    const std::string name = (s.size() > 5) ? trimCopy(s.substr(5)) : "";

    if (!isValidReplayName(name)) {
      session.postUiMessage("<MAGENTA>[!] Invalid replay name");
      return true;
    }

    const ReplayData replay = session.buildReplayData();
    if (ReplayIO::saveReplay(replay, name)) {
      running = false;
    } else {
      session.postUiMessage("<MAGENTA>[!] Failed to save replay");
    }
    return true;
  }

  return false;
}

}  // namespace scenes
