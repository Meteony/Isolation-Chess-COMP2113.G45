#pragma once

#include <ncurses.h>

#include <optional>
#include <string>

#include "core/replay_io.hpp"
#include "core/time.hpp"
#include "misc/key_queue.hpp"
#include "misc/settings_io.hpp"
#include "scenes/scene_common.hpp"
#include "sessions/replay_session.hpp"
#include "ui/board_renderer.hpp"
#include "ui/game_hud.hpp"
#include "ui/ui_resize_helper.hpp"

namespace scenes {

// Runs a replay scene for replayPath and returns an exit code.
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
      focus =
          (focus == FocusTarget::Game) ? FocusTarget::Hud : FocusTarget::Game;
    }

    const int gameInput =
        (focus == FocusTarget::Game && ch != '\t' && ch != '\x1b') ? ch : ERR;
    const int hudInput =
        (focus == FocusTarget::Hud && ch != '\t' && ch != '\x1b') ? ch : ERR;

    replay.update(gameInput);

    erase();
    board.render(gameInput, focus == FocusTarget::Game, replay);
    hud.render(hudInput, focus == FocusTarget::Hud, replay);

    if (std::optional<std::string> cmd =
            hud.consumeCommand()) { /*Handles message input*/
      focus = FocusTarget::Game;
      const std::string s = trimCopy(*cmd);

      if (!s.empty()) {
        if (s == ":q" || s == ":quit") {
          running = false;
        } else if (s == ":h" || s == ":help") {
          replay.postUiMessage(
              "<YELLOW>[i] Commands: :help, :quit, :reset, :play, "
              ":back, :forward, :goto <turn>, :speed <value>");
        } else if (s == ":reset") {
          replay.update('r');
          replay.postUiMessage("<YELLOW>[i] Reset.");
        } else if (s == ":play") {
          replay.update(' ');
          replay.postUiMessage("<YELLOW>[i] Autoplay toggled.");
        } else if (s == ":back") {
          replay.update('a');
          replay.postUiMessage("<YELLOW>[i] Back 1 step.");
        } else if (s == ":forward") {
          replay.update('d');
          replay.postUiMessage("<YELLOW>[i] Forward 1 step.");
        } else if (s.rfind(":goto", 0) == 0) {
          std::string arg = trimCopy(s.substr(5));

          if (!arg.empty() && arg.front() == '(' && arg.back() == ')') {
            arg = trimCopy(arg.substr(1, arg.size() - 2));
          }

          bool validNumber = !arg.empty();
          size_t turnNumber = 0;

          for (char ch : arg) {
            if (!std::isdigit(static_cast<unsigned char>(ch))) {
              validNumber = false;
              break;
            }
            turnNumber = turnNumber * 10 + static_cast<size_t>(ch - '0');
          }

          if (!validNumber) {
            replay.postUiMessage("<MAGENTA>[!] Usage: <MAGENTA>:goto <turn>");
          } else if (!replay.goToTurn(turnNumber)) {
            replay.postUiMessage("<MAGENTA>[!] Turn out of range");
          } else {
            replay.postUiMessage("<YELLOW>[i] Goto: Turn " +
                                 std::to_string(turnNumber) + ".");
          }
        } else if (s.rfind(":speed", 0) == 0) {
          std::string arg = trimCopy(s.substr(6));

          if (!arg.empty() && arg.front() == '(' && arg.back() == ')') {
            arg = trimCopy(arg.substr(1, arg.size() - 2));
          }

          try {
            size_t pos = 0;
            const float speed = std::stof(arg, &pos);

            if (pos != arg.size() || speed <= 0.0f) {
              replay.postUiMessage(
                  "<MAGENTA>[!] Usage: <MAGENTA>:speed <positive number>");
            } else {
              replay.setPlaybackSpeed(speed);
              replay.postUiMessage("<YELLOW>[i] Playback speed set to " +
                                   std::to_string(speed));
            }
          } catch (...) {
            replay.postUiMessage(
                "<MAGENTA>[!] Usage: <MAGENTA>:speed <positive number>");
          }
        } else if (s[0] == ':') {
          replay.postUiMessage("<MAGENTA>[!] Invalid command: " + s);
        } else {
          std::string tag = "Player";
          if (std::optional<Settings> settings = SettingsIO::loadSettings()) {
            if (!settings->gameTag.empty()) {
              tag = settings->gameTag;
            }
          }
          replay.postUiMessage("<P1>" + tag + ": " + s);
        }
      }
    }

    const int uiBottom = std::max(board.bottomRow(), hud.bottomRow());
    const int uiWidth = board.size().col + hud.size().col;
    if (focus == FocusTarget::Game) {
      drawBottomKeyTip(uiBottom, uiWidth,
                       {"[Tab/Esc] HUD", "[A/D] Step", "[Space] Autoplay",
                        "[R] Reset", "[:h] Help", "[:q] Quit"});
    } else {
      drawBottomKeyTip(uiBottom, uiWidth,
                       {"[Tab/Esc] Resume", "[↑↓] Scroll", "[←→] Cursor",
                        "[Enter] Run/Send", "[:h] Help"});
    }

    refresh();
    napms(kFrameMs);
  }

  return 0;
}

}  // namespace scenes
