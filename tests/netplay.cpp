#include <ncurses.h>

#include <algorithm>
#include <clocale>
#include <optional>
#include <string>

#include "core/enums.hpp"
#include "players/ai_player.hpp"
#include "players/human_player.hpp"
#include "players/network_player.hpp"
#include "sessions/match_session.hpp"
#include "ui/board_renderer.hpp"
#include "ui/game_hud.hpp"
#include "ui/ui_resize_helper.hpp"

enum class FocusTarget { Game, Hud };

int main() {
  std::setlocale(LC_ALL, "");

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(100);

  netplay::NetworkLink link;
  if (!link.connectTo("156.246.90.92", 5050, "room-123")) {
    std::cerr << link.lastError() << '\n';
    return 1;
  }

  while (!link.peerReady()) {
    std::string msg;
    while (link.popInfo(msg)) {
      std::cout << msg << '\n';
    }
    napms(100);
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

  MatchSession session(9, 11, p1, p2);

  constexpr int kBoardMaxRows = 20;
  constexpr int kBoardMaxCols = 48;
  constexpr int kHudMaxRows = 20;
  constexpr int kHudCols = 24;

  constexpr int kBoardStepRows = 2;
  constexpr int kBoardStepCols = 4;

  BoardRenderer board;
  GameHud hud;

  relayoutGameScene(board, hud);

  session.postUiMessage("P1: Ready.");
  session.postUiMessage("<MAGENTA> Goto: Turn 120");
  session.postUiMessage("[!] Invalid command");
  session.postUiMessage("P1: Moved to (3, 4) after 14s.");

  FocusTarget focus = FocusTarget::Game;

  bool running = true;
  while (running) {
    const int ch = getch();

    std::string msg;
    while (link.popInfo(msg)) {
      session.postUiMessage(msg);
    }

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

    if (std::optional<std::string> cmd = hud.consumeCommand()) {
      focus = FocusTarget::Game;
      if (cmd->empty()) {
        session.postUiMessage("[i] Returned to game");

      } else {
        if (cmd == ":quit") {
          running = false;
        }
        session.postUiMessage("Cmd: " + *cmd);
      }
    }

    refresh();
  }

  endwin();
  return 0;
}
