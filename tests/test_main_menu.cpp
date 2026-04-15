#include <ncurses.h>

#include <iostream>

#include "core/enums.hpp"
#include "core/replay_io.hpp"
#include "players/ai_player.hpp"
#include "players/human_player.hpp"
#include "sessions/match_session.hpp"
#include "ui/main_menu_scene.hpp"

// borrowed from manual_curses_match.cpp
static const char* phaseName(TurnPhase p) {
  switch (p) {
    case TurnPhase::NewTurn:
      return "NewTurn";
    case TurnPhase::Move:
      return "Move";
    case TurnPhase::Break:
      return "Break";
    default:
      return "?";
  }
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
           "WASD/Arrows move, Enter confirm, X cancel, Q quit");

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
  // init ncurses
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  // show the main menu first
  MainMenuScene menu;
  MenuChoice choice = menu.run();

  if (choice.mode == MenuChoice::Quit) {
    endwin();
    return 0;
  }

  // build the right players based on what was chosen
  Player* p1 = new HumanPlayer();
  Player* p2 = nullptr;

  if (choice.mode == MenuChoice::HumanVsHuman) {
    p2 = new HumanPlayer();
  } else {
    p2 = new AiPlayer(choice.difficulty, Side::Player2);
  }

  MatchSession session(7, 7, p1, p2);

  // switch to non-blocking for the game loop
  nodelay(stdscr, true);

  bool running = true;
  while (running) {
    timeout(100);
    int ch = getch();

    if (ch == 'q' || ch == 'Q') {
      running = false;
      continue;
    }

    session.update(ch);

    erase();
    drawBoard(session);
    refresh();
  }

  auto replayData = session.buildReplayData();
  std::string name = "I don't get it the name isn't respected anyways";
  bool ok = ReplayIO::saveReplay(replayData, name);
  endwin();
  std::cout << (ok ? "Replay saved\n" : "Error saving replay\n");
  return 0;
}
