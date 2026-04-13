#pragma once
#include <vector>

#include "game_state.hpp"
#include "turn_record.hpp"

struct ReplayData {
  GameState initialState;
  std::vector<TurnRecord> history;
};
