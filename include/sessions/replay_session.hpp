#pragma once
#include "core/game_state.hpp"
#include "core/turn_record.hpp"
#include <vector>
#include <cassert>
#include <cstddef>

enum class ReplayPhase {
  Move,
  Break,
  Finished
};

class ReplaySession {
  public:
  ReplaySession(const GameState& initialState, const std::vector<TurnRecord>& history);
  
  void reset(); //reset replay to beginning

  void applyCurrentMove();
  void applyCurrentBreak();

  bool hasNextAction() const;
  bool hasPreviousAction() const;

  bool stepForward(); //applies next action, false if none available
  bool stepBackward(); //undo last action, false if none available
  
  void replayToState(int targetTurnIndex, ReplayPhase targetPhase); //replay from initial state to given state
  

  const GameState& getCurrentState() const { return currentState_; }
  int getCurrentTurnIndex() const { return turnIndex_; }
  ReplayPhase getCurrentPhase() const { return phase_; }
  
  //bool applyFullTurn(); //move and break of next turn, true if full turn was applied
  

  private:
  GameState initialState_;
  std::vector<TurnRecord> history_;
  GameState currentState_;
  size_t turnIndex_;
  ReplayPhase phase_;
};
