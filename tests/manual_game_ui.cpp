#include <ncurses.h>

#include <algorithm>
#include <clocale>
#include <iostream>
#include <optional>
#include <string>

#include "core/enums.hpp"
#include "core/replay_io.hpp"
#include "misc/key_queue.hpp"
#include "misc/settings_io.hpp"
#include "players/ai_player.hpp"
#include "players/human_player.hpp"
#include "sessions/match_session.hpp"
#include "ui/board_renderer.hpp"
#include "ui/game_hud.hpp"
#include "ui/ui_resize_helper.hpp"

enum class FocusTarget { Game, Hud };

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " 1/2/3(Difficulty)\n";
    return 1;
  };

  std::string arg1 = argv[1];
  if (arg1 != "1" && arg1 != "2" && arg1 != "3") {
    std::cerr << "Invalid difficulty\n";
    return 1;
  }

  AiDifficulty difficulty;
  std::string aiName;
  if (arg1 == "1") {
    difficulty = AiDifficulty::Easy;
    aiName = "CPU (Easy)";
  } else if (arg1 == "2") {
    difficulty = AiDifficulty::Medium;
    aiName = "CPU (Medium)";
  } else if (arg1 == "3") {
    difficulty = AiDifficulty::Hard;
    aiName = "CPU (Hard)";
  }

  auto settings = SettingsIO::loadSettings();
  if (!settings) {
    Settings defaults{};
    if (!SettingsIO::saveSettings(defaults)) {
      return -1;
    }
    settings = defaults;
  }

  std::setlocale(LC_ALL, "");

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(0);

  MatchSession session(9, 11, new HumanPlayer(),
                       new AiPlayer(difficulty, Side::Player2),
                       settings->gameTag, aiName);

  constexpr int kBoardMaxRows = 20;
  constexpr int kBoardMaxCols = 48;
  constexpr int kHudMaxRows = 20;
  constexpr int kHudCols = 24;

  constexpr int kBoardStepRows = 2;
  constexpr int kBoardStepCols = 4;

  BoardRenderer board;
  GameHud hud;

  relayoutGameScene(board, hud);

  session.postUiMessage("<MAGENTA>[i] Game started.");

  FocusTarget focus = FocusTarget::Game;

  bool running = true;
  KeyQueue input;
  while (running) {
    const int ch = input.nextKeyOrErr();

    if (ch == KEY_RESIZE) {
      relayoutGameScene(board, hud);
    }

    // Tab/Esc switches focus between Game and HUD.
    if (ch == '\t' || ch == '\x1b') {
      focus =
          (focus == FocusTarget::Game) ? FocusTarget::Hud : FocusTarget::Game;
    }

    // Route input only to the focused side.
    const int gameInput = (focus == FocusTarget::Game && ch != '\t') ? ch : ERR;
    const int hudInput = (focus == FocusTarget::Hud && ch != '\t') ? ch : ERR;

    // Keep session ticking even when the HUD is focused.
    session.update(gameInput);

    erase();

    board.render(gameInput, focus == FocusTarget::Game, session);
    hud.render(hudInput, focus == FocusTarget::Hud, session);

    /*Parse command. Really chunky. Fold the next block.*/
    if (std::optional<std::string> cmd = hud.consumeCommand()) {
      focus = FocusTarget::Game;

      auto trim =
          [](std::string s) { /*Remove whitespace*/
                              while (!s.empty() &&
                                     std::isspace(static_cast<unsigned char>(
                                         s.front()))) {
                                s.erase(s.begin());
                              }
                              while (!s.empty() &&
                                     std::isspace(static_cast<unsigned char>(
                                         s.back()))) {
                                s.pop_back();
                              }
                              return s;
          };

      auto validReplayName =
          [](const std::string&
                 name) { /*Enforce legal chars*/
                         if (name.empty()) {
                           return true;  // empty => let ReplayIO auto-generate
                         }
                         if (name.size() > 32 || name == "." || name == "..") {
                           return false;
                         }
                         for (char ch : name) {
                           unsigned char uch = static_cast<unsigned char>(ch);
                           if (!(std::isalnum(uch) || ch == '_' || ch == '-' ||
                                 ch == '.')) {
                             return false;
                           }
                         }
                         return true;
          };

      std::string s = trim(*cmd);

      if (s == ":quit!") {
        running = false;
        continue;
      }

      if (s.compare(0, 6, ":quit!") == 0) {
        session.postUiMessage("<MAGENTA>[!] :quit! does not take an argument");
        continue;
      }

      /*3 blocks of help keytip. Fold those. */
      if (s == ":h" || s == ":help") {
        session.postUiMessage("<MAGENTA>[i] Commands:");
        session.postUiMessage("  :h / :help        Show this help");
        session.postUiMessage("  :save [name]      Save replay");
        session.postUiMessage("  :quit [name]      Save replay and quit");
        session.postUiMessage("  :quit!            Quit without saving");
        continue;
      }
      if (s.compare(0, 2, ":h") == 0 && s != ":h") {
        session.postUiMessage("<MAGENTA>[!] :h does not take an argument");
        continue;
      }
      if (s.compare(0, 5, ":help") == 0 && s != ":help") {
        session.postUiMessage("<MAGENTA>[!] :help does not take an argument");
        continue;
      }

      if (s == ":save" || (s.size() > 5 && s.compare(0, 5, ":save") == 0 &&
                           std::isspace(static_cast<unsigned char>(s[5])))) {
        std::string name = (s.size() > 5) ? trim(s.substr(5)) : "";

        if (!validReplayName(name)) {
          session.postUiMessage("<MAGENTA>[!] Invalid replay name");
          continue;
        }

        ReplayData replay = session.buildReplayData();
        if (ReplayIO::saveReplay(replay, name)) {
          session.postUiMessage("<MAGENTA>[i] Replay saved");
        } else {
          session.postUiMessage("<MAGENTA>[!] Failed to save replay");
        }
        continue;
      }

      if (s == ":quit" || (s.size() > 5 && s.compare(0, 5, ":quit") == 0 &&
                           std::isspace(static_cast<unsigned char>(s[5])))) {
        std::string name = (s.size() > 5) ? trim(s.substr(5)) : "";

        if (!validReplayName(name)) {
          session.postUiMessage("<MAGENTA>[!] Invalid replay name");
          continue;
        }

        ReplayData replay = session.buildReplayData();
        if (ReplayIO::saveReplay(replay, name)) {
          running = false;
        } else {
          session.postUiMessage("<MAGENTA>[!] Failed to save replay");
        }
        continue;
      }

      session.postUiMessage("<MAGENTA>[!] Invalid command: " + s);
    }

    refresh();
    napms(100);
  }

  endwin();
  return 0;
}
