#include <ncurses.h>

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "core/enums.hpp"
#include "misc/settings_io.hpp"
#include "scenes/live_match_scene.hpp"
#include "scenes/netplay_scene.hpp"
#include "scenes/replay_browser.hpp"
#include "scenes/replay_scene.hpp"

namespace {

enum class MainAction {
  HumanVsHuman,
  CpuEasy,
  CpuMedium,
  CpuHard,
  Netplay,
  ReplayBrowser,
  EditSettings,
  EditRoom,
  Exit,
};

std::string truncateToWidth(const std::string& s, int width) {
  if (width <= 0) {
    return "";
  }
  if (static_cast<int>(s.size()) <= width) {
    return s;
  }
  if (width <= 3) {
    return s.substr(0, width);
  }
  return s.substr(0, width - 3) + "...";
}

std::string padRight(std::string s, int width) {
  if (static_cast<int>(s.size()) < width) {
    s.append(width - s.size(), ' ');
  }
  return s;
}

void drawBox(int top, int left, int height, int width,
             const std::string& title) {
  mvaddch(top, left, ACS_ULCORNER);
  for (int x = 1; x < width - 1; ++x) {
    mvaddch(top, left + x, ACS_HLINE);
  }
  mvaddch(top, left + width - 1, ACS_URCORNER);

  for (int y = 1; y < height - 1; ++y) {
    mvaddch(top + y, left, ACS_VLINE);
    mvaddch(top + y, left + width - 1, ACS_VLINE);
  }

  mvaddch(top + height - 1, left, ACS_LLCORNER);
  for (int x = 1; x < width - 1; ++x) {
    mvaddch(top + height - 1, left + x, ACS_HLINE);
  }
  mvaddch(top + height - 1, left + width - 1, ACS_LRCORNER);

  if (!title.empty()) {
    mvprintw(top, left + 2, "%s", title.c_str());
  }
}

void drawButtonRow(int row, int left, int width, int digit,
                   const std::string& label) {
  const std::string prefix = "[" + std::to_string(digit) + "] ";
  const std::string body =
      padRight(prefix + truncateToWidth(label, width - 4), width - 2);
  mvprintw(row, left + 1, "%s", body.c_str());
}

void waitForAnyKey() {
  nodelay(stdscr, FALSE);
  getch();
  nodelay(stdscr, TRUE);
}

void showMessageBox(const std::string& title,
                    const std::vector<std::string>& lines) {
  erase();

  int maxRows = 0;
  int maxCols = 72;
  getmaxyx(stdscr, maxRows, maxCols);

  int innerWidth = 24;
  for (const std::string& line : lines) {
    innerWidth = std::max(innerWidth, static_cast<int>(line.size()) + 2);
  }
  innerWidth = std::min(innerWidth, std::max(24, maxCols - 4));

  const int width = std::min(maxCols - 2, innerWidth + 2);
  const int height = std::min(maxRows - 2, static_cast<int>(lines.size()) + 4);
  const int top = std::max(0, (maxRows - height) / 2);
  const int left = std::max(0, (maxCols - width) / 2);

  drawBox(top, left, height, width, " " + title + " ");
  for (int i = 0; i < height - 4 && i < static_cast<int>(lines.size()); ++i) {
    mvprintw(top + 2 + i, left + 2, "%s",
             truncateToWidth(lines[i], width - 4).c_str());
  }
  mvprintw(top + height - 2, left + 2, "%s", "Press any key to continue.");
  refresh();
  waitForAnyKey();
}

std::optional<std::string> promptLineInput(const std::string& title,
                                           const std::string& currentValue,
                                           const std::string& hint) {
  erase();
  int maxRows = 0;
  int maxCols = 0;
  getmaxyx(stdscr, maxRows, maxCols);

  const int width = std::max(50, std::min(76, maxCols - 4));
  const int height = 8;
  const int top = std::max(0, (maxRows - height) / 2);
  const int left = std::max(0, (maxCols - width) / 2);
  drawBox(top, left, height, width, " " + title + " ");
  mvprintw(top + 2, left + 2, "%s", truncateToWidth(hint, width - 4).c_str());
  mvprintw(top + 3, left + 2, "%s",
           truncateToWidth("Current: " + currentValue, width - 4).c_str());
  mvprintw(top + 5, left + 2, "%s", "> ");

  nodelay(stdscr, FALSE);
  echo();
  curs_set(1);
  char buffer[128] = {0};
  mvgetnstr(top + 5, left + 4, buffer, static_cast<int>(sizeof(buffer) - 1));
  curs_set(0);
  noecho();
  nodelay(stdscr, TRUE);

  return std::string(buffer);
}

bool parseIntStrict(const std::string& s, int& outValue) {
  if (s.empty()) {
    return false;
  }
  try {
    std::size_t pos = 0;
    const int value = std::stoi(s, &pos);
    if (pos != s.size()) {
      return false;
    }
    outValue = value;
    return true;
  } catch (...) {
    return false;
  }
}

bool isValidIpv4(const std::string& ip) {
  if (ip.empty()) {
    return false;
  }

  int parts = 0;
  std::string current;
  for (char ch : ip) {
    if (ch == '.') {
      int value = 0;
      if (current.empty() || !parseIntStrict(current, value) || value < 0 ||
          value > 255) {
        return false;
      }
      current.clear();
      ++parts;
      continue;
    }

    if (!std::isdigit(static_cast<unsigned char>(ch))) {
      return false;
    }
    current.push_back(ch);
  }

  int value = 0;
  if (current.empty() || !parseIntStrict(current, value) || value < 0 ||
      value > 255) {
    return false;
  }
  ++parts;
  return parts == 4;
}

bool isValidServerIp(const std::string& ip) {
  return ip == "localhost" || isValidIpv4(ip);
}

bool isValidServerPort(int port) { return port >= 1 && port <= 65535; }

bool isValidTag(const std::string& tag) {
  if (tag.empty() || tag.size() > 24) {
    return false;
  }
  for (char ch : tag) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    if (!(std::isalnum(uch) || ch == '_' || ch == '-' || ch == '.')) {
      return false;
    }
  }
  return true;
}

bool isValidRoomCode(const std::string& room) {
  if (room.empty() || room.size() > 32) {
    return false;
  }
  for (char ch : room) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    if (!(std::isalnum(uch) || ch == '_' || ch == '-' || ch == '.')) {
      return false;
    }
  }
  return true;
}

void editSettingsMenu(Settings& settings) {
  Settings working = settings;

  while (true) {
    erase();
    int maxRows = 0;
    int maxCols = 0;
    getmaxyx(stdscr, maxRows, maxCols);
    (void)maxRows;

    const int width = std::min(78, maxCols - 2);
    const int left = std::max(0, (maxCols - width) / 2);
    drawBox(1, left, 11, width, " Settings ");

    drawButtonRow(3, left + 1, width - 2, 1,
                  "Server IP      : " + working.serverIp);
    drawButtonRow(4, left + 1, width - 2, 2,
                  "Server Port    : " + std::to_string(working.serverPort));
    drawButtonRow(5, left + 1, width - 2, 3,
                  "Game Tag       : " + working.gameTag);
    drawButtonRow(6, left + 1, width - 2, 4, "Reset to defaults");
    drawButtonRow(7, left + 1, width - 2, 5, "Save and return");
    drawButtonRow(8, left + 1, width - 2, 6, "Discard and return");

    mvprintw(10, left + 2, "%s",
             "IP accepts localhost or IPv4. Tag uses A-Z a-z 0-9 _ - .");
    refresh();

    const int ch = getch();
    if (ch == ERR) {
      napms(16);
      continue;
    }

    if (ch == '1') {
      const std::optional<std::string> value = promptLineInput(
          "Edit server IP", working.serverIp,
          "Leave blank to cancel. Examples: localhost, 127.0.0.1");
      if (value && !value->empty()) {
        if (isValidServerIp(*value)) {
          working.serverIp = *value;
        } else {
          showMessageBox("Invalid server IP",
                         {"Use localhost or a valid IPv4 address."});
        }
      }
    } else if (ch == '2') {
      const std::optional<std::string> value = promptLineInput(
          "Edit server port", std::to_string(working.serverPort),
          "Leave blank to cancel. Valid range: 1 - 65535");
      if (value && !value->empty()) {
        int port = 0;
        if (parseIntStrict(*value, port) && isValidServerPort(port)) {
          working.serverPort = port;
        } else {
          showMessageBox("Invalid server port",
                         {"Enter an integer from 1 to 65535."});
        }
      }
    } else if (ch == '3') {
      const std::optional<std::string> value = promptLineInput(
          "Edit game tag", working.gameTag,
          "Leave blank to cancel. Max 24 chars. Allowed: A-Z a-z 0-9 _ - .");
      if (value && !value->empty()) {
        if (isValidTag(*value)) {
          working.gameTag = *value;
        } else {
          showMessageBox("Invalid game tag",
                         {"Use 1-24 characters from A-Z a-z 0-9 _ - ."});
        }
      }
    } else if (ch == '4') {
      working = Settings{};
    } else if (ch == '5') {
      if (SettingsIO::saveSettings(working)) {
        settings = working;
        return;
      }
      showMessageBox("Save failed",
                     {"The settings could not be saved to settings.cfg."});
    } else if (ch == '6') {
      return;
    }
  }
}

void editRoomCode(std::string& roomCode) {
  const std::optional<std::string> value =
      promptLineInput("Edit room code", roomCode,
                      "Leave blank to cancel. Allowed: A-Z a-z 0-9 _ - .");
  if (!value || value->empty()) {
    return;
  }
  if (!isValidRoomCode(*value)) {
    showMessageBox("Invalid room code",
                   {"Use 1-32 characters from A-Z a-z 0-9 _ - ."});
    return;
  }
  roomCode = *value;
}

std::optional<std::string> replayBrowserMenu() {
  int page = 0;

  while (true) {
    const std::vector<scenes::ReplaySummary> replays = scenes::scanReplays();
    const int pageSize = 7;
    const int totalPages = std::max(
        1, (static_cast<int>(replays.size()) + pageSize - 1) / pageSize);
    page = std::clamp(page, 0, totalPages - 1);

    erase();
    int maxRows = 0;
    int maxCols = 0;
    getmaxyx(stdscr, maxRows, maxCols);
    (void)maxRows;

    const int width = std::min(90, maxCols - 2);
    const int left = std::max(0, (maxCols - width) / 2);
    drawBox(1, left, 16, width, " Replay Browser ");

    const int start = page * pageSize;
    const int end =
        std::min(start + pageSize, static_cast<int>(replays.size()));

    for (int i = start; i < end; ++i) {
      const scenes::ReplaySummary& replay = replays[i];
      const int digit = (i - start) + 1;
      const std::string label = replay.fileName + " | winner: " + replay.winner;
      drawButtonRow(3 + (i - start), left + 1, width - 2, digit, label);
    }

    if (replays.empty()) {
      mvprintw(4, left + 2, "%s", "No replay files were found in ./replays.");
    }

    drawButtonRow(11, left + 1, width - 2, 8, "Return to main menu");
    drawButtonRow(12, left + 1, width - 2, 9,
                  (page > 0) ? "Previous page" : "Previous page (unavailable)");
    drawButtonRow(
        13, left + 1, width - 2, 0,
        (page + 1 < totalPages) ? "Next page" : "Next page (unavailable)");

    if (!replays.empty() && start < static_cast<int>(replays.size())) {
      const scenes::ReplaySummary& preview = replays[start];
      mvprintw(15, left + 2, "%s",
               truncateToWidth("Preview: " + preview.player1Name + " vs " +
                                   preview.player2Name + " | turns: " +
                                   std::to_string(preview.turns) +
                                   " | board: " + std::to_string(preview.rows) +
                                   "x" + std::to_string(preview.cols),
                               width - 4)
                   .c_str());
    }

    refresh();
    const int ch = getch();
    if (ch == ERR) {
      napms(16);
      continue;
    }

    if (ch >= '1' && ch <= '7') {
      const int index = start + (ch - '1');
      if (index < static_cast<int>(replays.size())) {
        return replays[index].path;
      }
    } else if (ch == '8') {
      return std::nullopt;
    } else if (ch == '9') {
      if (page > 0) {
        --page;
      }
    } else if (ch == '0') {
      if (page + 1 < totalPages) {
        ++page;
      }
    }
  }
}

MainAction mainMenu(const Settings& settings, const std::string& roomCode) {
  while (true) {
    erase();
    int maxRows = 0;
    int maxCols = 72;
    getmaxyx(stdscr, maxRows, maxCols);
    (void)maxRows;

    const int width = std::min(82, maxCols - 2);
    const int left = std::max(0, (maxCols - width) / 2);

    drawBox(1, left, 14, width, " Isolation Chess Launcher ");
    mvprintw(3, left + 2, "%s",
             truncateToWidth("Player tag: " + settings.gameTag +
                                 "   |   Netplay room: " + roomCode,
                             width - 4)
                 .c_str());

    drawButtonRow(5, left + 1, width - 2, 1, "Human vs Human");
    drawButtonRow(6, left + 1, width - 2, 2, "Human vs CPU - Easy");
    drawButtonRow(7, left + 1, width - 2, 3, "Human vs CPU - Medium");
    drawButtonRow(8, left + 1, width - 2, 4, "Human vs CPU - Hard");
    drawButtonRow(9, left + 1, width - 2, 5, "Netplay");
    drawButtonRow(10, left + 1, width - 2, 6, "Replay browser");
    drawButtonRow(11, left + 1, width - 2, 7, "Edit settings");
    drawButtonRow(12, left + 1, width - 2, 8, "Edit room code");
    drawButtonRow(13, left + 1, width - 2, 0, "Exit");

    refresh();
    const int ch = getch();
    if (ch == ERR) {
      napms(16);
      continue;
    }

    switch (ch) {
      case '1':
        return MainAction::HumanVsHuman;
      case '2':
        return MainAction::CpuEasy;
      case '3':
        return MainAction::CpuMedium;
      case '4':
        return MainAction::CpuHard;
      case '5':
        return MainAction::Netplay;
      case '6':
        return MainAction::ReplayBrowser;
      case '7':
        return MainAction::EditSettings;
      case '8':
        return MainAction::EditRoom;
      case '0':
        return MainAction::Exit;
      default:
        break;
    }
  }
}

}  // namespace

int main() {
  std::optional<Settings> loaded = SettingsIO::loadSettings();
  if (!loaded) {
    Settings defaults{};
    SettingsIO::saveSettings(defaults);
    loaded = defaults;
  }

  Settings settings = *loaded;
  std::string roomCode = "room-123";
  bool running = true;

  while (running) {
    MainAction action = MainAction::Exit;
    std::string replayToOpen;

    {
      scenes::NcursesGuard menuCurses;

      while (true) {
        action = mainMenu(settings, roomCode);

        if (action == MainAction::EditSettings) {
          editSettingsMenu(settings);
          continue;
        }

        if (action == MainAction::EditRoom) {
          editRoomCode(roomCode);
          continue;
        }

        if (action == MainAction::ReplayBrowser) {
          const std::optional<std::string> replayPath = replayBrowserMenu();
          if (!replayPath) {
            continue;
          }
          replayToOpen = *replayPath;
        }

        break;
      }
    }

    switch (action) {
      case MainAction::HumanVsHuman:
        scenes::runHumanVsHuman(settings);
        break;
      case MainAction::CpuEasy:
        scenes::runHumanVsCpu(settings, AiDifficulty::Easy);
        break;
      case MainAction::CpuMedium:
        scenes::runHumanVsCpu(settings, AiDifficulty::Medium);
        break;
      case MainAction::CpuHard:
        scenes::runHumanVsCpu(settings, AiDifficulty::Hard);
        break;
      case MainAction::Netplay:
        scenes::runNetplay(settings, roomCode);
        break;
      case MainAction::ReplayBrowser:
        if (!replayToOpen.empty()) {
          scenes::runReplay(replayToOpen);
        }
        break;
      case MainAction::EditSettings:
      case MainAction::EditRoom:
        break;
      case MainAction::Exit:
        running = false;
        break;
    }
  }

  return 0;
}
