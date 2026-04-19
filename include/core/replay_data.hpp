#pragma once
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
