#include <ncurses.h>

#include "core/enums.hpp"
#include "core/game_state.hpp"
#include "players/ai_player.hpp"
#include "players/human_player.hpp"
#include "sessions/match_session.hpp"

static const char* phaseName(TurnPhase p) {
  switch (p) {
    case TurnPhase::NewTurn:
      return "NewTurn";
    case TurnPhase::Move:
      return "Move";
    case TurnPhase::Break:
      return "Break";
  }
  return "?";
}

static const char* sideName(Side s) {
  return (s == Side::Player1) ? "P1" : "P2";
}

static void drawBoard(const MatchSession& session) {
  const GameState& state = session.state();

  for (int r = 0; r < state.rows(); ++r) {
    for (int c = 0; c < state.cols(); ++c) {
      Coord here{r, c};
      char ch = '.';

      if (state.tileAt(here) == TileState::Broken) ch = '#';
      if (here == state.playerPos(Side::Player1)) ch = '1';
      if (here == state.playerPos(Side::Player2)) ch = '2';

      mvaddch(r, c * 2, ch);
      mvaddch(r, c * 2 + 1, ' ');
    }
  }

  mvaddstr(state.rows() + 1, 0,
           "WASD/Arrows move cursor, C or Enter confirm, X cancel, q quit");

  std::string info = std::string("Turn: ") + sideName(state.sideToMove()) +
                     " | Phase: " + phaseName(state.phase());
  mvaddstr(state.rows() + 2, 0, info.c_str());

  if (session.isFinished()) {
    std::string endMsg = std::string("Winner: ") + sideName(state.winner());
    mvaddstr(state.rows() + 3, 0, endMsg.c_str());
  }

  const auto& vs = session.visualState();
  if (vs.cursorVisible) {
    mvaddch(vs.cursor.row, vs.cursor.col * 2 + 1, '<');
  }
}

int main() {
  initscr();
  cbreak();
  nodelay(stdscr, true);
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  MatchSession session(7, 7, new HumanPlayer(),
                       new HumanPlayer()  // swap to AiPlayer later if desired
  );

  bool running = true;
  while (running) {
    timeout(100);
    // erase();

    int ch = getch();
    if (ch == 'q' || ch == 'Q') {
      running = false;
      continue;
    }

    session.update(ch);
    drawBoard(session);

    refresh();
  }

  endwin();
  return 0;
}
