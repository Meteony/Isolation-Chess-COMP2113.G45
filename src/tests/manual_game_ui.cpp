#include <ncurses.h>

#include <clocale>
#include <optional>
#include <string>

#include "core/enums.hpp"
#include "players/ai_player.hpp"
#include "players/human_player.hpp"
#include "sessions/match_session.hpp"
#include "ui/board_renderer.hpp"
#include "ui/game_hud.hpp"

enum class FocusTarget { Game, Hud };

int main() {
  std::setlocale(LC_ALL, "");

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(100);

  MatchSession session(9, 11, new HumanPlayer(),
                       new AiPlayer(AiDifficulty::Medium, Side::Player2));

  BoardRenderer board;
  board.moveTo(0, 0);
  board.resize(20, 47);

  GameHud hud;
  hud.moveTo(0, 48);
  hud.resize(20, 24);

  session.postUiMessage("P1: Ready.");
  session.postUiMessage("<MAGENTA> Goto: Turn 120");
  session.postUiMessage("[!] Invalid command");
  session.postUiMessage("P1: Moved to (3, 4) after 14s.");

  FocusTarget focus = FocusTarget::Game;

  bool running = true;
  while (running) {
    const int ch = getch();

    if (ch == 'q' || ch == 'Q') {
      running = false;
      continue;
    }

    // Tab switches focus between Game and HUD.
    if (ch == '\t') {
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
      if (cmd->empty()) {
        session.postUiMessage("[i] Returned to game");
      } else {
        session.postUiMessage("Cmd: " + *cmd);
      }
    }

    refresh();
  }

  endwin();
  return 0;
}
