#pragma once

#include <ncurses.h>

#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "core/enums.hpp"
#include "core/time.hpp"
#include "misc/blizzard_transition.hpp"
#include "misc/key_queue.hpp"
#include "misc/settings_io.hpp"
#include "players/network_player.hpp"
#include "scenes/scene_common.hpp"
#include "sessions/match_session.hpp"
#include "ui/board_renderer.hpp"
#include "ui/game_hud.hpp"
#include "ui/ui_resize_helper.hpp"

namespace scenes {

constexpr int kCanvasMaxRows = 21;
constexpr int kCanvasMaxCols = 71;

// Draws a centered status box with title and lines.
inline void drawCenteredStatusBox(const std::string& title,
                                  const std::vector<std::string>& lines) {
  erase();

  int maxRows = 0;
  int maxCols = 0;
  getmaxyx(stdscr, maxRows, maxCols);

  /*
  maxRows = std::min(maxRows, kCanvasMaxRows);
  maxCols = std::min(maxCols, kCanvasMaxCols);
*/
  int innerWidth = 0;
  for (const std::string& line : lines) {
    innerWidth = std::max(innerWidth, static_cast<int>(line.size()));
  }
  innerWidth = std::max(innerWidth, static_cast<int>(title.size()));
  innerWidth = std::max(innerWidth + 2, 32);

  const int boxWidth = innerWidth + 2;
  const int boxHeight = static_cast<int>(lines.size()) + 4;
  const int top = std::max(0, (maxRows - boxHeight) / 2);
  const int left = std::max(0, (maxCols - boxWidth) / 2);

  ensureUiColorsInitialized();
  attron(COLOR_PAIR(CP_FRAME_FOCUSED));

  mvaddstr(top, left, "╭");
  mvaddstr(top, left + boxWidth - 1, "╮");
  mvaddstr(top + boxHeight - 1, left, "╰");
  mvaddstr(top + boxHeight - 1, left + boxWidth - 1, "╯");

  for (int x = 1; x < boxWidth - 1; ++x) {
    mvaddstr(top, left + x, "─");
    mvaddstr(top + boxHeight - 1, left + x, "─");
  }

  for (int y = 1; y < boxHeight - 1; ++y) {
    mvaddstr(top + y, left, "│");
    mvaddstr(top + y, left + boxWidth - 1, "│");
  }

  attroff(COLOR_PAIR(CP_FRAME_FOCUSED));

  mvprintw(top, left + 2, "%s", title.c_str());
  for (std::size_t i = 0; i < lines.size(); ++i) {
    mvprintw(top + 2 + static_cast<int>(i), left + 2, "%s", lines[i].c_str());
  }

  refresh();
}

// Builds the two network-aware players and transfers ownership to MatchSession.
// Keep this here instead of MatchSession so the core session does not need to
// include the platform-specific netplay player header.
inline MatchSession createNetplayMatchSession(netplay::NetworkLink& link) {
  Player* p1 = nullptr;
  Player* p2 = nullptr;

  if (link.mySide() == Side::Player1) {
    p1 = new netplay::NetworkHumanPlayer(link);
    p2 = new netplay::NetworkPlayer(link);
  } else {
    p1 = new netplay::NetworkPlayer(link);
    p2 = new netplay::NetworkHumanPlayer(link);
  }

  return MatchSession::TakeOwnership(9, 11, p1, p2, link.player1Tag(),
                                     link.player2Tag());
}

// Runs the netplay scene for roomCode and returns an exit code.
inline int runNetplay(const Settings& settings, const std::string& roomCode,
                      BlizzardEffect* effect = nullptr) {
  netplay::NetworkLink link;
  drawCenteredStatusBox(" Netplay ",
                        {"Connecting to " + settings.serverIp + ":" +
                             std::to_string(settings.serverPort),
                         "Room: " + roomCode, "Tag:  " + settings.gameTag});

  if (!link.connectTo(settings.serverIp, settings.serverPort, roomCode,
                      settings.gameTag)) {
    drawCenteredStatusBox(" Netplay Error ",
                          {link.lastError(), "", "Press any key to return."});
    nodelay(stdscr, FALSE);
    getch();
    nodelay(stdscr, TRUE);
    return 1;
  }

  std::vector<std::string> waitingLog;
  while (!link.peerReady()) {
    std::string msg;
    while (link.popInfo(msg)) {
      waitingLog.push_back(msg);
      if (waitingLog.size() > 6) {
        waitingLog.erase(waitingLog.begin());
      }
    }

    if (link.hasFatalError()) {
      drawCenteredStatusBox(" Netplay Error ",
                            {link.lastError(), "", "Press any key to return."});
      nodelay(stdscr, FALSE);
      getch();
      nodelay(stdscr, TRUE);
      link.disconnect();
      return 1;
    }

    std::vector<std::string> lines;
    lines.push_back("Waiting for the other player...");
    lines.push_back("Room: " + roomCode);
    lines.push_back("");
    lines.insert(lines.end(), waitingLog.begin(), waitingLog.end());
    drawCenteredStatusBox(" Netplay ", lines);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  MatchSession session = createNetplayMatchSession(link);
  BoardRenderer board;
  GameHud hud;
  relayoutGameScene(board, hud);

  session.postUiMessage("<YELLOW>[i] Netplay connected.");
  session.postUiMessage("<YELLOW>[i] " + link.player1Tag() + " vs " +
                        link.player2Tag());

  FocusTarget focus = FocusTarget::Game;
  KeyQueue input;
  bool running = true;
  bool fatalShown = false;

  if (effect) effect->startBlockingTransition();

  while (running) {
    const int ch = input.nextKeyOrErr();

    std::string msg;
    // Debug [net] messages
    /*
    while (link.popInfo(msg)) {
      session.postUiMessage("<YELLOW>" + msg);
    }
    */

    netplay::NetChat chat;
    while (link.popChat(chat)) {
      const std::string color = (chat.side == Side::Player1) ? "<P1>" : "<P2>";
      session.postUiMessage(color + session.playerName(chat.side) + ": " +
                            chat.text);
    }

    if (link.hasFatalError() && !fatalShown) {
      session.postUiMessage("<MAGENTA>[!] Network error: " + link.lastError());
      fatalShown = true;
    }

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

    const int uiBottom = std::max(board.bottomRow(), hud.bottomRow());
    const int uiWidth = board.size().col + hud.size().col;
    if (focus == FocusTarget::Game) {
      drawBottomKeyTip(
          uiBottom, uiWidth,
          {"[Tab] Chat", "[WASD] Move", "[C] Confirm", "[Arrows] Scroll"});
    } else {
      drawBottomKeyTip(
          uiBottom, uiWidth,
          {"[Tab] Back", "[:h] Help", "[Arrows] Edit", "[Enter] Send"});
    }

    if (std::optional<std::string> cmd = hud.consumeCommand()) {
      focus = FocusTarget::Game;

      if (handleStandardSaveQuitCommand(session, *cmd, running)) {
        goto RefreshAndSleep;
      }

      const std::string s = trimCopy(*cmd);
      if (!s.empty() && s[0] == ':') {
        session.postUiMessage("<MAGENTA>[!] Invalid command: " + s);
        goto RefreshAndSleep;
      }

      if (!link.sendChat(s)) {
        session.postUiMessage("<MAGENTA>[!] Failed to send chat");
        goto RefreshAndSleep;
      }

      const std::string myColor =
          (link.mySide() == Side::Player1) ? "<P1>" : "<P2>";
      session.postUiMessage(myColor + session.playerName(link.mySide()) + ": " +
                            s);
    }

  RefreshAndSleep:
    if (effect) effect->updateAndDraw();
    refresh();
    napms(kFrameMs);
  }

  link.disconnect();
  // if (effect) effect->startBlockingTransition();
  return 0;
}

}  // namespace scenes
