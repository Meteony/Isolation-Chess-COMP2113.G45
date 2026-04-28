#pragma once

#include <ncurses.h>

#include <core/time.hpp>
#include <optional>
#include <string>

#include "core/enums.hpp"
#include "misc/blizzard_transition.hpp"
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

// Runs one already-created live match scene and returns an exit code.
inline int runLiveMatchSession(MatchSession& session,
                               BlizzardEffect* effect = nullptr) {
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
      const std::string s = trimCopy(*cmd);

      if (!s.empty()) {
        const bool handled = handleStandardSaveQuitCommand(session, s, running);
        if (handled) {
          // nothing else
        } else if (s[0] == ':') {
          session.postUiMessage("<YELLOW>[!] Invalid command: " + s);
        } else {
          std::string tag = "Player";
          if (std::optional<Settings> settings = SettingsIO::loadSettings()) {
            if (!settings->gameTag.empty()) {
              tag = settings->gameTag;
            }
          }
          session.postUiMessage("<P1>" + tag + ": " + s);
        }
      }
    }

    const int uiBottom = std::max(board.bottomRow(), hud.bottomRow());
    const int uiWidth = board.size().col + hud.size().col;
    if (focus == FocusTarget::Game) {
      drawBottomKeyTip(
          uiBottom, uiWidth,
          {"[Tab] HUD", "[WASD] Move", "[C] Confirm", "[Arrows] Scroll"});
    } else {
      drawBottomKeyTip(
          uiBottom, uiWidth,
          {"[Tab] Back", "[:h] Help", "[Arrows] Edit", "[Enter] Send"});
    }

    if (effect) effect->updateAndDraw();

    refresh();
    napms(kFrameMs);
  }

  // if (effect) effect->startBlockingTransition();
  return 0;
}

// Runs a local human-vs-human match from settings.
inline int runHumanVsHuman(const Settings& settings,
                           BlizzardEffect* effect = nullptr) {
  MatchSession session =
      MatchSession::CreateHumanVsHuman(9, 11, settings.gameTag, "Player 2");
  return runLiveMatchSession(session, effect);
}

// Returns the display label for an AI difficulty.
inline std::string difficultyLabel(AiDifficulty difficulty) {
  switch (difficulty) {
    case AiDifficulty::Easy:
      return "CPU(Easy)";
    case AiDifficulty::Medium:
      return "CPU(Medium)";
    case AiDifficulty::Hard:
      return "CPU(Hard)";
  }
  return "CPU";
}

// Runs a local human-vs-CPU match from settings.
inline int runHumanVsCpu(const Settings& settings, AiDifficulty difficulty,
                         BlizzardEffect* effect = nullptr) {
  MatchSession session = MatchSession::CreateHumanVsAi(
      9, 11, difficulty, settings.gameTag, difficultyLabel(difficulty));
  return runLiveMatchSession(session, effect);
}

}  // namespace scenes
