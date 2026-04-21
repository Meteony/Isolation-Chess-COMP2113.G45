#pragma once
#include <optional>
#include <string>
#include <vector>

#include "game_state.hpp"
#include "turn_record.hpp"

struct ReplayData {
  GameState initialState;
  std::vector<TurnRecord> history;
  std::vector<std::string> uiMessages;

  int winner; /*-1 = no winner, 0 = Player1, 1 = Player2*/

  std::string player1Name;
  std::string player2Name;
};

struct ReplayMetadata {
  int rows = -1;
  int cols = -1;
  int winner = -1;  // -1 none, 0 p1, 1 p2
  std::string player1Name;
  std::string player2Name;
};

static std::optional<ReplayMetadata> loadReplayMetadata(
    const std::string& filepath);
