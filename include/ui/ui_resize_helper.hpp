#pragma once

#include <ncurses.h>

#include <algorithm>

#include "board_renderer.hpp"
#include "game_hud.hpp"

struct GameSceneLayoutConfig {
  int boardMaxRows = 20;
  int boardMaxCols = 47;

  int hudMaxRows = 20;
  int hudCols = 24;

  int boardStepRows = 2;
  int boardStepCols = 4;

  // 2 border rows + 2 tile rows
  int minBoardRows = 4;

  // 2 border cols + 1 left pad + 4 tile cols
  int minBoardCols = 7;
};

inline void relayoutGameScene(BoardRenderer& board, GameHud& hud,
                              const GameSceneLayoutConfig& cfg = {}) {
  int screenRows = 0;
  int screenCols = 0;
  getmaxyx(stdscr, screenRows, screenCols);

  int boardRows = cfg.boardMaxRows;
  int boardCols = cfg.boardMaxCols;

  // Shrink board width first in 4-char tile steps.
  if (screenCols < cfg.boardMaxCols + cfg.hudCols) {
    const int missingCols = cfg.boardMaxCols + cfg.hudCols - screenCols;
    const int shrinkStepsX =
        (missingCols + cfg.boardStepCols - 1) / cfg.boardStepCols;
    boardCols -= shrinkStepsX * cfg.boardStepCols;
  }

  // Shrink board height in 2-row tile steps.
  if (screenRows < cfg.boardMaxRows) {
    const int missingRows = cfg.boardMaxRows - screenRows;
    const int shrinkStepsY =
        (missingRows + cfg.boardStepRows - 1) / cfg.boardStepRows;
    boardRows -= shrinkStepsY * cfg.boardStepRows;
  }

  boardCols = std::max(cfg.minBoardCols, boardCols);
  boardRows = std::max(cfg.minBoardRows, boardRows);

  board.moveTo(0, 0);
  board.resize(boardRows, boardCols);

  hud.moveTo(0, boardCols);
  hud.resize(boardRows, cfg.hudCols);
}
