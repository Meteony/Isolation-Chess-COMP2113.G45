#include "sessions/replay_session.hpp"

#include <cassert>

#include "core/game_rules.hpp"

ReplaySession::ReplaySession(const GameState& initialState,
                             const std::vector<TurnRecord>& history)
    : r_state(initialState),
      r_initialState(initialState),
      r_history(history),
      r_turnIndex(0)  // member initializer list
{
  updateVisualState();
}

void ReplaySession::update(int inputChar) {
  // input handling
  switch (inputChar) {
    case 'a':
    case 'A':  // backward
      setAutoPlay(false);
      stepBackward();
      break;

    case 'd':
    case 'D':  // forward
      setAutoPlay(false);
      stepForward();
      break;

    case ' ':  // space for autoplay on/off
      toggleAutoPlay();
      break;

    case 'r':
    case 'R':  // reset to start
      setAutoPlay(false);
      reset();
      break;
  }

  r_autoPlayTargetDelay =
      calculateAutoPlayDelay();  // update target delay based on current turn's
                                 // think times and playback speed

  // autoplay tick tracking
  if (r_autoPlayActive && !isFinished()) {
    r_autoPlayCounter++;
    if (r_autoPlayCounter >= r_autoPlayTargetDelay) {
      r_autoPlayCounter = 0;
      if (!stepForward()) {  // reached end
        setAutoPlay(false);
      }
    }
  }

  updateVisualState();
}

const GameState& ReplaySession::state() const { return r_state; }

TurnPhase ReplaySession::phase() const { return r_state.phase(); }

const std::vector<TurnRecord>& ReplaySession::history() const {
  return r_history;
}

const ReplayVisualState& ReplaySession::visualState() const {
  return r_visualState;
}

bool ReplaySession::isFinished() const {
  return r_state.status() == SessionStatus::Finished;
}

void ReplaySession::reset() {
  r_state = r_initialState;
  r_turnIndex = 0;
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

bool ReplaySession::currentTurnValid() const {
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
  if (!currentTurnValid()) {
    return false;
  }

  const TurnRecord& curTurn = r_history[r_turnIndex];
  r_state.setSideToMove(curTurn.actor);

  if (r_state.phase() == TurnPhase::NewTurn) {
    r_state.setPhase(TurnPhase::Move);
    return true;
  }

  if (r_state.phase() == TurnPhase::Move) {
    applyCurrentMove();
    r_state.setPhase(TurnPhase::Break);
    return true;
  }

  if (r_state.phase() == TurnPhase::Break) {
    applyCurrentBreak();

    bool hasAnotherTurn = (r_turnIndex + 1 < r_history.size());
    if (!hasAnotherTurn) {
      r_state.setStatus(SessionStatus::Finished);
      r_state.setWinner(r_state.sideToMove());
      setAutoPlay(false);
      return true;
    }

    ++r_turnIndex;
    r_state.setPhase(TurnPhase::NewTurn);
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

  if (isFinished()) {
    assert(!r_history.empty() &&
           "ReplaySession::stepBackward: empty r_history");
    targetTurn = r_history.size() - 1;
    targetPhase = TurnPhase::Break;
  }

  else if (r_state.phase() ==
           TurnPhase::NewTurn) {  // go to break phase of previous turn
    if (r_turnIndex == 0) {       // no previous turn
      return false;
    }
    targetTurn = r_turnIndex - 1;
    targetPhase = TurnPhase::Break;
  }

  else if (r_state.phase() == TurnPhase::Move) {
    targetPhase = TurnPhase::NewTurn;
    // targetTurn stays the same
  }

  else if (r_state.phase() ==
           TurnPhase::Break) {  // go back to move phase of same turn
    targetPhase = TurnPhase::Move;
    // targetTurn stays the same
  }

  replayToState(targetTurn, targetPhase);
  return true;
}

void ReplaySession::replayToState(size_t targetTurnIndex,
                                  TurnPhase targetPhase) {
  reset();

  // replay forward until reach desired (turnIndex, phase)
  while (r_turnIndex < targetTurnIndex ||
         (r_turnIndex == targetTurnIndex && r_state.phase() != targetPhase)) {
    if (!stepForward()) {
      break;
    }
  }
}

void ReplaySession::updateVisualState() {
  r_visualState.currentTurn = r_turnIndex;
  r_visualState.totalTurn = r_history.size();
  r_visualState.canStepForward = currentTurnValid();
  r_visualState.canStepBackward = hasPreviousAction();
  r_visualState.isAutoPlaying = r_autoPlayActive;
  r_visualState.playbackSpeed = r_playbackSpeed;
  if (r_turnIndex < r_history.size()) {
    const TurnRecord& currentTurn = r_history[r_turnIndex];
    if (r_state.phase() == TurnPhase::Move) {
      r_visualState.currentTurnThinkTime = currentTurn.thinkTicksBeforeMove;
    }

    else if (r_state.phase() == TurnPhase::Break) {
      r_visualState.currentTurnThinkTime = currentTurn.thinkTicksBeforeBreak;
    }

    else {
      r_visualState.currentTurnThinkTime = 0;
    }
  } else {
    r_visualState.currentTurnThinkTime = 0;
  }
}

void ReplaySession::setAutoPlay(bool active) {
  r_autoPlayActive = active;
  r_autoPlayCounter = 0;  // reset counter
}

long ReplaySession::calculateAutoPlayDelay() const {
  if (r_history.empty() || r_turnIndex >= r_history.size()) {
    return r_defaultAutoPlayDelay;  // empty history or end of history, use
                                    // default delay
  }

  const TurnRecord& currentTurn = r_history[r_turnIndex];
  long baseDelay = 0;
  if (r_state.phase() == TurnPhase::Move) {
    baseDelay = currentTurn.thinkTicksBeforeMove;
  }

  else if (r_state.phase() == TurnPhase::Break) {
    baseDelay = currentTurn.thinkTicksBeforeBreak;
  }

  else {
    baseDelay = r_defaultAutoPlayDelay;  // for NewTurn or Finished phases, use
                                         // default delay
  }

  long adjustedDelay = static_cast<long>(baseDelay / r_playbackSpeed);
  return adjustedDelay > 0
             ? adjustedDelay
             : r_defaultAutoPlayDelay;  // ensure positive delay, fallback to
                                        // default if adjusted delay is zero or
                                        // negative
}

void ReplaySession::setAutoPlayDelay(
    int ticks) {  // for manually setting delay (not currently used)
  r_autoPlayTargetDelay = ticks;
}

void ReplaySession::setPlaybackSpeed(float speed) {
  r_playbackSpeed =
      speed > 0 ? speed
                : 1.0f;  // ensure positive speed, fallback to normal if invalid
  r_autoPlayTargetDelay =
      calculateAutoPlayDelay();  // recalculate target delay based on new
                                 // playback speed
}

bool ReplaySession::isAutoPlayActive() const { return r_autoPlayActive; }

void ReplaySession::toggleAutoPlay() { setAutoPlay(!r_autoPlayActive); }
