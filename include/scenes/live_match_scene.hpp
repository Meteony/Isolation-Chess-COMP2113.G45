#pragma once

#include <ncurses.h>

#include <core/time.hpp>
#include <optional>
#include <string>

#include "core/enums.hpp"
#include "misc/key_queue.hpp"
#include "misc/settings_io.hpp"
#include "players/ai_player.hpp"
#include "players/human_player.hpp"
#include "scenes/scene_common.hpp"
#include "sessions/match_session.hpp"
#include "ui/board_renderer.hpp"
#include "ui/game_hud.hpp"
#include "ui/ui_resize_helper.hpp"

namespace scenes {

inline int runLiveMatchSession(Player* p1, Player* p2,
                               const std::string& player1Name,
                               const std::string& player2Name) {
  NcursesGuard curses;

  MatchSession session(9, 11, p1, p2, player1Name, player2Name);
  BoardRenderer board;
  GameHud hud;
  relayoutGameScene(board, hud);

  session.postUiMessage("<YELLOW>[i] Match started.");

  FocusTarget focus = FocusTarget::Game;
  KeyQueue input;
  bool running = true;

  while (running) {
    const int ch = input.nextKeyOrErr();

    if (ch == KEY_RESIZE) {
      relayoutGameScene(board, hud);
    }

    if (ch == '\t' || ch == '\x1b') {
      focus =
          (focus == FocusTarget::Game) ? FocusTarget::Hud : FocusTarget::Game;
    }

    const int gameInput =
        (focus == FocusTarget::Game && ch != '\t' && ch != '\x1b') ? ch : ERR;
    const int hudInput =
        (focus == FocusTarget::Hud && ch != '\t' && ch != '\x1b') ? ch : ERR;

    session.update(gameInput);

    erase();
    board.render(gameInput, focus == FocusTarget::Game, session);
    hud.render(hudInput, focus == FocusTarget::Hud, session);

    if (std::optional<std::string> cmd = hud.consumeCommand()) {
      focus = FocusTarget::Game;
      const bool handled =
          handleStandardSaveQuitCommand(session, *cmd, running);
      if (!handled) {
        session.postUiMessage("<YELLOW>[!] Invalid command: " + trimCopy(*cmd));
      }
    }

    refresh();
    napms(kFrameMs);
  }

  return 0;
}

inline int runHumanVsHuman(const Settings& settings) {
  return runLiveMatchSession(new HumanPlayer(), new HumanPlayer(),
                             settings.gameTag, "Player 2");
}

inline std::string difficultyLabel(AiDifficulty difficulty) {
  switch (difficulty) {
    case AiDifficulty::Easy:
      return "CPU (Easy)";
    case AiDifficulty::Medium:
      return "CPU (Medium)";
    case AiDifficulty::Hard:
      return "CPU (Hard)";
  }
  return "CPU";
}

inline int runHumanVsCpu(const Settings& settings, AiDifficulty difficulty) {
  return runLiveMatchSession(new HumanPlayer(),
                             new AiPlayer(difficulty, Side::Player2),
                             settings.gameTag, difficultyLabel(difficulty));
}

}  // namespace scenes
