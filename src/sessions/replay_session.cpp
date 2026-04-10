#include "sessions/replay_session.hpp"
#include "core/game_rules.hpp"

ReplaySession::ReplaySession(const GameState& initialState, const std::vector<TurnRecord>& history)
  : initialState_(initialState)
  , history_(history)
  , currentState_(initialState)
  , turnIndex_(0)
  , phase_(history.empty() ? ReplayPhase::Finished : ReplayPhase::Move) // member initializer list
{
}

void ReplaySession::reset() {
  currentState_ = initialState_;
  turnIndex_ = 0;
  phase_ = history_.empty() ? ReplayPhase::Finished : ReplayPhase::Move;
}

void ReplaySession::applyCurrentMove() {
  assert(turnIndex_ >= 0 && turnIndex_ < static_cast<int>(history_.size()));
  const TurnRecord& turn = history_[turnIndex_];
  GameRules::applyMove(currentState_, turn.actor, turn.moveCoord);
}

void ReplaySession::applyCurrentBreak() {
  assert(turnIndex_ >= 0 && turnIndex_ < static_cast<int>(history_.size()));
  const TurnRecord& turn = history_[turnIndex_];
  GameRules::applyBreak(currentState_, turn.breakCoord);
}

bool ReplaySession::hasNextAction() const {
  if (phase_ == ReplayPhase::Finished) { //return false if replay explicitly finished
    return false;
  }

  if (turnIndex_ >= history_.size()) { //return false if all turns consumed
    return false;
  }

  return true;
}

bool ReplaySession::hasPreviousAction() const {
  if (history_.empty()) {
    return false;
  }
  if (turnIndex_ == 0 && phase_ == ReplayPhase::Move) {
    return false;
  }
  return true;
}

bool ReplaySession::stepForward() {
  if (!hasNextAction()) { //return false if no next action
    return false;
  }
  
  if (phase_ == ReplayPhase::Move) { //apply the move
    applyCurrentMove();
    phase_ = ReplayPhase::Break;
    return true;
  }

  else if (phase_ == ReplayPhase::Break) {
    applyCurrentBreak();
    turnIndex_++;
    phase_ = (turnIndex_ < static_cast<int>(history_.size())) ? ReplayPhase::Move : ReplayPhase::Finished;
    return true;
  }
  
  return false;
}

bool ReplaySession::stepBackward() {
  if (!hasPreviousAction()) {
    return false;
  }

  int targetTurn = turnIndex_;
  ReplayPhase targetPhase = phase_;

  if (phase_ == ReplayPhase::Move) { //go to break phase of previous turn
    if (turnIndex_ == 0) { //no previous turn
      return false;
    }
    targetTurn = turnIndex_ - 1;
    targetPhase = ReplayPhase::Break;
  }

  else if (phase_ == ReplayPhase::Break) { //go back to move phase of same turn
    targetPhase = ReplayPhase::Move;
    //targetTurn stays the same
  }

  else { //ReplayPhase::Finished
    if (history_.empty()) {
      return false;
    }
    targetTurn = static_cast<int>(history_.size()) - 1;
    targetPhase = ReplayPhase::Break;
  }

  replayToState(targetTurn, targetPhase);
  return true;
}

void ReplaySession::replayToState(int targetTurnIndex, ReplayPhase targetPhase) {
  reset();

  //replay forward until reach desired (turnIndex, phase)
  while (turnIndex_ < targetTurnIndex || (turnIndex_ == targetTurnIndex && phase_ != targetPhase)) {
    if (!hasNextAction()) {
      break;
    }

    if (phase_ == ReplayPhase:: Move) {
      applyCurrentMove();
      phase_ = ReplayPhase::Break;
    }

    else if (phase_ == ReplayPhase::Break) {
      applyCurrentBreak();
      turnIndex_++;
      phase_ = (turnIndex_ < static_cast<int>(history_.size())) ? ReplayPhase::Move : ReplayPhase::Finished;
    }
  }
}
