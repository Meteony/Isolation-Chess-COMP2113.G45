#include "sessions/replay_session.hpp"
#include "core/game_rules.hpp"
#include <cassert>

ReplaySession::ReplaySession(const GameState& initialState, const std::vector<TurnRecord>& history)
  : r_state(initialState)
  , r_initialState(initialState)
  , r_history(history)
  , r_turnIndex(0) // member initializer list
{
  updateVisualState();
}


void ReplaySession::update (int inputChar) {
  //input handling
  switch (inputChar) {
    case 'a':
    case 'A': //backward
      setAutoPlay(false);
      stepBackward();
      break;

    case 'd':
    case 'D': //forward
      setAutoPlay(false);
      stepForward();
      break;

    case ' ': //space for autoplay on/off
      toggleAutoPlay();
      break;

    case 'r':
    case 'R': //reset to start
      setAutoPlay(false);
      reset();
      break;
  }

  //autoplay tick tracking
  if (r_autoPlayActive && !isFinished()) {
    r_autoPlayCounter++;
    if (r_autoPlayCounter >= r_autoPlayDelayTicks) {
      r_autoPlayCounter = 0;
      if (!stepForward()) { //reached end
        setAutoPlay(false);
      }
    }
  }

  updateVisualState();
}

const GameState& ReplaySession::state() const {
  return r_state;
}

TurnPhase ReplaySession::phase() const {
  return r_state.phase();
}

const std::vector<TurnRecord>& ReplaySession::history() const {
  return r_history;
}

const ReplayVisualState& ReplaySession::visualState() const {
  return r_visualState;
}

bool ReplaySession::isFinished() const {
  return r_state.phase() == TurnPhase::Finished;
}

void ReplaySession::reset() {
  r_state = r_initialState;
  r_turnIndex = 0;
  r_state.setPhase( r_history.empty() ? TurnPhase::Finished : TurnPhase::NewTurn );
  updateVisualState();
}

void ReplaySession::applyCurrentMove() {
  assert(r_turnIndex < r_history.size());
  const TurnRecord& turn = r_history[r_turnIndex];
  GameRules::applyMove(r_state, turn.actor, turn.moveCoord);
}

void ReplaySession::applyCurrentBreak() {
  assert(r_turnIndex < r_history.size());
  const TurnRecord& turn = r_history[r_turnIndex];
  GameRules::applyBreak(r_state, turn.breakCoord);
}

bool ReplaySession::hasNextAction() const {
  if (isFinished() || r_turnIndex >= r_history.size()) { 
    return false;
  }

  return true;
}

bool ReplaySession::hasPreviousAction() const {
  if (r_history.empty()) {
    return false;
  }
  if (r_turnIndex == 0 && r_state.phase() == TurnPhase::NewTurn) {
    return false;
  }
  return true;
}

bool ReplaySession::stepForward() {
  if (!hasNextAction()) { //return false if no next action
    return false;
  }
  
  if (r_state.phase() == TurnPhase::NewTurn) {
    r_state.setPhase(TurnPhase::Move);
    return true;
  }

  else if (r_state.phase() == TurnPhase::Move) { //apply the move
    applyCurrentMove();
    r_state.setPhase(TurnPhase::Break);
    return true;
  }

  else if (r_state.phase() == TurnPhase::Break) {
    applyCurrentBreak();
    ++r_turnIndex;
    r_state.setPhase((r_turnIndex < r_history.size()) ? TurnPhase::NewTurn : TurnPhase::Finished);
    return true;
  }
  
  return false;
}

bool ReplaySession::stepBackward() {
  if (!hasPreviousAction()) {
    return false;
  }

  size_t targetTurn = r_turnIndex;
  TurnPhase targetPhase = r_state.phase();

  if (r_state.phase() == TurnPhase::NewTurn) { //go to break phase of previous turn
    if (r_turnIndex == 0) { //no previous turn
      return false;
    }
    targetTurn = r_turnIndex - 1;
    targetPhase = TurnPhase::Break;
  }

  else if (r_state.phase() == TurnPhase::Move) {
    targetPhase = TurnPhase::NewTurn;
    //targetTurn stays the same
  }

  else if (r_state.phase() == TurnPhase::Break) { //go back to move phase of same turn
    targetPhase = TurnPhase::Move;
    //targetTurn stays the same
  }

  else { //ReplayPhase::Finished
    if (r_history.empty()) {
      return false;
    }
    targetTurn = r_history.size() - 1;
    targetPhase = TurnPhase::Break;
  }

  replayToState(targetTurn, targetPhase);
  return true;
}

void ReplaySession::replayToState(size_t targetTurnIndex, TurnPhase targetPhase) {
  reset();

  //replay forward until reach desired (turnIndex, phase)
  while (r_turnIndex < targetTurnIndex || (r_turnIndex == targetTurnIndex && r_state.phase() != targetPhase)) {
    if (!stepForward()) {
      break;
    }
  }
}

void ReplaySession::updateVisualState() {
  r_visualState.currentTurn = r_turnIndex;
  r_visualState.totalTurn = r_history.size();
  r_visualState.canStepForward = hasNextAction();
  r_visualState.canStepBackward = hasPreviousAction();
  r_visualState.isAutoPlaying = r_autoPlayActive;
}

void ReplaySession::setAutoPlay(bool active) {
  r_autoPlayActive = active;
  r_autoPlayCounter = 0; //reset counter
}

void ReplaySession::setAutoPlayDelay(int ticks) {
  r_autoPlayDelayTicks = ticks;
}

bool ReplaySession::isAutoPlayActive() const {
  return r_autoPlayActive;
}

void ReplaySession::toggleAutoPlay() {
  setAutoPlay(!r_autoPlayActive);
}
