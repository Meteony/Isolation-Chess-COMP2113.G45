#pragma once

#include <ncurses.h>

#include <optional>
#include <string>

#include "core/replay_io.hpp"
#include "misc/key_queue.hpp"
#include "scenes/scene_common.hpp"
#include "sessions/replay_session.hpp"
#include "ui/board_renderer.hpp"
#include "ui/game_hud.hpp"
#include "ui/ui_resize_helper.hpp"

namespace scenes {

inline int runReplay(const std::string& replayPath) {
  const std::optional<ReplayData> replayData = ReplayIO::loadReplay(replayPath);
  if (!replayData) {
    return 1;
  }

  NcursesGuard curses;

  ReplaySession replay(*replayData);
  BoardRenderer board;
  GameHud hud;
  relayoutGameScene(board, hud);

  FocusTarget focus = FocusTarget::Game;
  KeyQueue input;
  bool running = true;

  while (running) {
    const int ch = input.nextKeyOrErr();

    if (ch == KEY_RESIZE) {
      relayoutGameScene(board, hud);
    }

    if (ch == '\t' || ch == '\x1b') {
      focus = (focus == FocusTarget::Game) ? FocusTarget::Hud
                                           : FocusTarget::Game;
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

      if (cmd->empty() || *cmd == ":q" || *cmd == ":quit") {
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
    napms(100);
  }

  return 0;
}

}  // namespace scenes
