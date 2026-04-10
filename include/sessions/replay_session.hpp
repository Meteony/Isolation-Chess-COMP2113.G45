#pragma once
#include "core/game_state.hpp"
#include "core/turn_record.hpp"
#include "core/enums.hpp"
#include <vector>
#include <cassert>
#include <cstddef>

struct ReplayVisualState { //ui element
  size_t currentTurn = 0;
  size_t totalTurn = 0;
  bool canStepForward = false;
  bool canStepBackward = false;
  bool isAutoPlaying = false;
};

class ReplaySession {
private:
  GameState r_state;
  GameState r_initialState;

  std::vector<TurnRecord> r_history;
  size_t r_turnIndex;
  
  ReplayVisualState r_visualState;

  //auto-play state
  bool r_autoPlayActive = false;
  int r_autoPlayDelayTicks = 30;
  int r_autoPlayCounter = 0;

private:
  //helper methods
  void updateVisualState();
  void reset(); //reset replay to beginning
  bool hasNextAction() const;
  bool hasPreviousAction() const;
  bool stepForward(); //applies next action, false if none available
  bool stepBackward(); //undo last action, false if none available
  void replayToState(size_t targetTurnIndex, TurnPhase targetPhase); //replay from initial state to given state
  void toggleAutoPlay();
  void applyCurrentMove();
  void applyCurrentBreak();

public:
  ReplaySession(const GameState& initialState, const std::vector<TurnRecord>& history);
  
  //updated new methods (similar to MatchSession)
  void update(int inputChar);
  const GameState& state() const;
  TurnPhase phase() const;
  const std::vector<TurnRecord>& history() const;
  const ReplayVisualState& visualState() const;
  bool isFinished() const;

  //auto-play methods
  void setAutoPlay(bool active);
  void setAutoPlayDelay(int ticks);
  bool isAutoPlayActive() const;
};
