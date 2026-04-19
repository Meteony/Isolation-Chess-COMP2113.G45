#include <ncurses.h>

#include <cctype>
#include <chrono>
#include <clocale>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

#include "core/enums.hpp"
#include "core/replay_io.hpp"
#include "misc/key_queue.hpp"
#include "misc/settings_io.hpp"
#include "players/network_player.hpp"
#include "sessions/match_session.hpp"
#include "ui/board_renderer.hpp"
#include "ui/game_hud.hpp"
#include "ui/ui_resize_helper.hpp"

enum class FocusTarget { Game, Hud };

int main() {
  auto settings = SettingsIO::loadSettings();
  if (!settings) {
    std::cerr << "Failed to load settings.cfg\n";
    return 1;
  }

  netplay::NetworkLink link;
  if (!link.connectTo(settings->serverIp, settings->serverPort, "room-123",
                      settings->gameTag)) {
    std::cerr << link.lastError() << '\n';
    return 1;
  }

  while (!link.peerReady()) {
    std::string msg;
    while (link.popInfo(msg)) {
      std::cout << msg << '\n';
    }

    if (link.hasFatalError()) {
      std::cerr << link.lastError() << '\n';
      return 1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  Player* p1 = nullptr;
  Player* p2 = nullptr;

  if (link.mySide() == Side::Player1) {
    p1 = new netplay::NetworkHumanPlayer(link);
    p2 = new netplay::NetworkPlayer(link);
  } else {
    p1 = new netplay::NetworkPlayer(link);
    p2 = new netplay::NetworkHumanPlayer(link);
  }

  MatchSession session(9, 11, p1, p2, link.player1Tag(), link.player2Tag());

  std::setlocale(LC_ALL, "");

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(0);

  BoardRenderer board;
  GameHud hud;
  relayoutGameScene(board, hud);

  session.postUiMessage("<MAGENTA>[i] Netplay connected.");
  session.postUiMessage("<MAGENTA>[i] " + link.player1Tag() + " vs " +
                        link.player2Tag());

  FocusTarget focus = FocusTarget::Game;
  bool running = true;
  bool fatalShown = false;
  KeyQueue input;

  while (running) {
    const int ch = input.nextKeyOrErr();

    std::string msg;
    while (link.popInfo(msg)) {
      session.postUiMessage("<MAGENTA>" + msg);
    }

    netplay::NetChat chat;
    while (link.popChat(chat)) {
      const std::string color =
          (chat.side == Side::Player1) ? "<BLUE>" : "<RED>";
      session.postUiMessage(color + session.playerName(chat.side) + ": " +
                            chat.text);
    }

    if (link.hasFatalError() && !fatalShown) {
      session.postUiMessage("<MAGENTA>[!] Network error: " + link.lastError());
      fatalShown = true;
    }

    if (ch == KEY_RESIZE) {
      relayoutGameScene(board, hud);
    }

    if (ch == '\t' || ch == '\x1b') {
      focus =
          (focus == FocusTarget::Game) ? FocusTarget::Hud : FocusTarget::Game;
    }

    const int gameInput = (focus == FocusTarget::Game && ch != '\t') ? ch : ERR;
    const int hudInput = (focus == FocusTarget::Hud && ch != '\t') ? ch : ERR;

    session.update(gameInput);

    erase();
    board.render(gameInput, focus == FocusTarget::Game, session);
    hud.render(hudInput, focus == FocusTarget::Hud, session);

    if (std::optional<std::string> cmd = hud.consumeCommand()) {
      focus = FocusTarget::Game;

      auto trim = [](std::string s) {
        while (!s.empty() &&
               std::isspace(static_cast<unsigned char>(s.front()))) {
          s.erase(s.begin());
        }
        while (!s.empty() &&
               std::isspace(static_cast<unsigned char>(s.back()))) {
          s.pop_back();
        }
        return s;
      };

      auto validReplayName = [](const std::string& name) {
        if (name.empty()) return true;
        if (name.size() > 32 || name == "." || name == "..") return false;
        for (char ch : name) {
          unsigned char uch = static_cast<unsigned char>(ch);
          if (!(std::isalnum(uch) || ch == '_' || ch == '-' || ch == '.')) {
            return false;
          }
        }
        return true;
      };

      std::string s = trim(*cmd);

      if (s.empty()) {
        session.postUiMessage("<MAGENTA>[i] Returned to game");
        refresh();
        napms(100);
        continue;
      }

      if (s == ":quit!") {
        running = false;
        continue;
      }

      if (s.compare(0, 6, ":quit!") == 0) {
        session.postUiMessage("<MAGENTA>[!] :quit! does not take an argument");
        refresh();
        napms(100);
        continue;
      }

      if (s == ":h" || s == ":help") {
        session.postUiMessage("<MAGENTA>[i] Commands:");
        session.postUiMessage("  :h / :help        Show this help");
        session.postUiMessage("  :save [name]      Save replay");
        session.postUiMessage("  :quit [name]      Save replay and quit");
        session.postUiMessage("  :quit!            Quit without saving");
        session.postUiMessage("  <text>            Send chat");
        refresh();
        napms(100);
        continue;
      }

      if (s.compare(0, 2, ":h") == 0 && s != ":h") {
        session.postUiMessage("<MAGENTA>[!] :h does not take an argument");
        refresh();
        napms(100);
        continue;
      }

      if (s.compare(0, 5, ":help") == 0 && s != ":help") {
        session.postUiMessage("<MAGENTA>[!] :help does not take an argument");
        refresh();
        napms(100);
        continue;
      }

      if (s == ":save" || (s.size() > 5 && s.compare(0, 5, ":save") == 0 &&
                           std::isspace(static_cast<unsigned char>(s[5])))) {
        std::string name = (s.size() > 5) ? trim(s.substr(5)) : "";

        if (!validReplayName(name)) {
          session.postUiMessage("<MAGENTA>[!] Invalid replay name");
          refresh();
          napms(100);
          continue;
        }

        ReplayData replay = session.buildReplayData();
        if (ReplayIO::saveReplay(replay, name)) {
          session.postUiMessage("<MAGENTA>[i] Replay saved");
        } else {
          session.postUiMessage("<MAGENTA>[!] Failed to save replay");
        }

        refresh();
        napms(100);
        continue;
      }

      if (s == ":quit" || (s.size() > 5 && s.compare(0, 5, ":quit") == 0 &&
                           std::isspace(static_cast<unsigned char>(s[5])))) {
        std::string name = (s.size() > 5) ? trim(s.substr(5)) : "";

        if (!validReplayName(name)) {
          session.postUiMessage("<MAGENTA>[!] Invalid replay name");
          refresh();
          napms(100);
          continue;
        }

        ReplayData replay = session.buildReplayData();
        if (ReplayIO::saveReplay(replay, name)) {
          running = false;
        } else {
          session.postUiMessage("<MAGENTA>[!] Failed to save replay");
        }

        refresh();
        napms(100);
        continue;
      }

      if (!s.empty() && s[0] == ':') {
        session.postUiMessage("<MAGENTA>[!] Invalid command: " + s);
        refresh();
        napms(100);
        continue;
      }

      if (!link.sendChat(s)) {
        session.postUiMessage("<MAGENTA>[!] Failed to send chat");
        refresh();
        napms(100);
        continue;
      }

      const std::string myColor =
          (link.mySide() == Side::Player1) ? "<BLUE>" : "<RED>";
      session.postUiMessage(myColor + session.playerName(link.mySide()) + ": " +
                            s);
    }

    refresh();
    napms(100);
  }

  endwin();
  link.disconnect();
  return 0;
}
