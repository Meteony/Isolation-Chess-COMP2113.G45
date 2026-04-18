#include <ncurses.h>

#include <clocale>

#include "players/human_player.hpp"
#include "sessions/match_session.hpp"
#include "ui/board_renderer.hpp"

int main() {
  std::setlocale(LC_ALL, "");

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(100);

  MatchSession session(9, 11,  // rows, cols  -> 11 x 9 board
                       new HumanPlayer(), new HumanPlayer());

  BoardRenderer renderer;
  renderer.moveTo(0, 0);
  renderer.resize(20, 48);

  bool running = true;
  while (running) {
    const int ch = getch();

    if (ch == 'q' || ch == 'Q') {
      running = false;
      continue;
    }

    erase();

    session.update(ch);
    renderer.render(ch, true, session);

    mvaddstr(
        22, 0,
        "Q quit | WASD/Arrows/Enter control players | HJKL scroll renderer");

    refresh();
  }

  endwin();
  return 0;
}
