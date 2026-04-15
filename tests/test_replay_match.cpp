#include <iostream>
#include <ncurses.h>
#include "sessions/replay_session.hpp"
#include "core/replay_io.hpp"
#include "core/game_state.hpp"
#include "core/enums.hpp"

static const char* phaseName(TurnPhase p) {
    switch (p) {
    case TurnPhase::NewTurn: return "NewTurn";
    case TurnPhase::Move:    return "Move";
    case TurnPhase::Break:   return "Break";
    default:                 return "?";
    }
}

static const char* sideName(Side s) {
    return (s == Side::Player1) ? "P1" : "P2";
}

static void drawBoard(const ReplaySession& replay) {
    const GameState& state = replay.state();

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

    mvaddstr(state.rows() + 1, 0, "A back, D forward, Space autoplay, R reset, Q quit");

    std::string info = std::string("Turn: ") + sideName(state.sideToMove()) +
                       " | Phase: " + phaseName(state.phase());
    mvaddstr(state.rows() + 2, 0, info.c_str());

    const auto& vs = replay.visualState();

    std::string replayInfo =
        "Replay turn: " + std::to_string(vs.currentTurn) +
        "/" + std::to_string(vs.totalTurn) +
        " | autoplay: " + std::string(vs.isAutoPlaying ? "on" : "off") +
        " | speed: " + std::to_string(vs.playbackSpeed);

    mvaddstr(state.rows() + 3, 0, replayInfo.c_str());

    std::string thinkInfo =
        "Current think time: " + std::to_string(vs.currentTurnThinkTime);
    mvaddstr(state.rows() + 4, 0, thinkInfo.c_str());

    if (replay.isFinished()) {
        std::string endMsg = std::string("Winner: ") + sideName(state.winner());
        mvaddstr(state.rows() + 5, 0, endMsg.c_str());
    }
}


int main() {
    std::string replayFile {};
    std::cout << "Replay Path: replays/";
    std::getline(std::cin, replayFile);
    const std::string replayPath = "replays/" + replayFile;

    auto replayData = ReplayIO::loadReplay(replayPath);
    if (!replayData) {
        std::cerr << "Failed to load replay: " << replayPath << "\n";
        return 1;
    }

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    timeout(100);

    ReplaySession replay(replayData->initialState, replayData->history);

    bool running = true;
    while (running) {
        int ch = getch();

        if (ch == 'q' || ch == 'Q') {
            running = false;
            continue;
        }

        replay.update(ch);

        erase();
        drawBoard(replay);
        refresh();
    }

    endwin();
    return 0;
}
