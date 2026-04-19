#include <ncurses.h>

#include <clocale>
#include <iostream>
#include <optional>
#include <string>

#include "core/replay_io.hpp"
#include "sessions/replay_session.hpp"
#include "ui/board_renderer.hpp"
#include "ui/game_hud.hpp"
#include "ui/ui_resize_helper.hpp"

enum class FocusTarget { Game, Hud };

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " replays/<file>\n";
    return 1;
  }

  auto replayData = ReplayIO::loadReplay(argv[1]);
  if (!replayData) {
    std::cerr << "Failed to load replay: " << argv[1] << "\n";
    return 1;
  }

  std::setlocale(LC_ALL, "");

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(100);

  ReplaySession replay(*replayData);

  BoardRenderer board;
  GameHud hud;

  FocusTarget focus = FocusTarget::Game;
  bool running = true;

  relayoutGameScene(board, hud);

  while (running) {
    const int ch = getch();

    if (ch == KEY_RESIZE) {
      relayoutGameScene(board, hud);
    }

    if (ch == '\t' || ch == '\x1b') {
      focus =
          (focus == FocusTarget::Game) ? FocusTarget::Hud : FocusTarget::Game;
    }

    if (focus == FocusTarget::Game && (ch == 'q' || ch == 'Q')) {
      running = false;
      continue;
    }

    const int gameInput =
        (focus == FocusTarget::Game && ch != '\t' && ch != '\x1b') ? ch : ERR;
    const int hudInput =
        (focus == FocusTarget::Hud && ch != '\t' && ch != '\x1b') ? ch : ERR;

    replay.update(gameInput);

    erase();
    board.render(gameInput, focus == FocusTarget::Game, replay);
    hud.render(hudInput, focus == FocusTarget::Hud, replay);

    if (std::optional<std::string> cmd = hud.consumeCommand()) {
      focus = FocusTarget::Game;

      if (cmd->empty()) {
        // Just leave HUD command mode.
      } else if (*cmd == ":quit" || *cmd == ":q") {
        running = false;
      } else if (*cmd == ":reset") {
        replay.update('r');
      } else if (*cmd == ":play") {
        replay.update(' ');
      } else if (*cmd == ":back") {
        replay.update('a');
      } else if (*cmd == ":forward") {
        replay.update('d');
      }
    }

    refresh();
  }

  endwin();
  return 0;
}
