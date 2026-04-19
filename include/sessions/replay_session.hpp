#pragma once
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include "core/enums.hpp"
#include "core/game_state.hpp"
#include "core/replay_data.hpp"
#include "core/time.hpp"
#include "core/turn_record.hpp"

struct ReplayVisualState {  // ui element
  size_t currentTurn = 0;
  size_t totalTurn = 0;
  bool canStepForward = false;
  bool canStepBackward = false;
  bool isAutoPlaying = false;
  long currentTurnThinkTime = 0;  // in ticks
  float playbackSpeed =
      1.0f;  // for display purposes, shows current playback speed
};

class ReplaySession {
 private:
  std::vector<std::string> m_uiMessages{};

  std::string m_player1Name{"Player 1"};
  std::string m_player2Name{"Player 2"};

  GameState r_state;
  GameState r_initialState;

  std::vector<TurnRecord> r_history;
  size_t r_turnIndex;

  ReplayVisualState r_visualState;

  // auto-play state & timing
  bool r_autoPlayActive = false;
  int r_defaultAutoPlayDelay = 0.3 * kGameFps;  // fallback delay ticks
  int r_autoPlayCounter = 0;
  long r_autoPlayTargetDelay = 0;
  float r_playbackSpeed =
      1.0f;  // 1.0 = normal speed, <1.0 = slower, >1.0 = faster

 private:
  // helper methods
  void updateVisualState();
  void reset();  // reset replay to beginning
  bool currentTurnValid() const;
  bool hasPreviousAction() const;
  bool stepForward();   // applies next action, false if none available
  bool stepBackward();  // undo last action, false if none available
  void replayToState(
      size_t targetTurnIndex,
      TurnPhase targetPhase);  // replay from initial state to given state
  void applyCurrentMove();
  void applyCurrentBreak();

  void toggleAutoPlay();
  long calculateAutoPlayDelay() const;  // calculate delay based on current
                                        // turn's think times and playback speed

 public:
  explicit ReplaySession(const ReplayData& data);

  const std::string& playerName(Side side) const;

  void postUiMessage(const std::string& msg) {
    if (m_uiMessages.size() >= 256) {
      m_uiMessages.erase(m_uiMessages.begin());
    }
    m_uiMessages.push_back(msg);
  }
  const std::vector<std::string>& uiMessages() const { return m_uiMessages; }

  // updated new methods (similar to MatchSession)
  void update(int inputChar);
  const GameState& state() const;
  TurnPhase phase() const;
  const std::vector<TurnRecord>& history() const;
  const ReplayVisualState& visualState() const;
  bool isFinished() const;

  // auto-play methods
  void setAutoPlay(bool active);
  void setAutoPlayDelay(int ticks);
  bool isAutoPlayActive() const;
  void setPlaybackSpeed(float speed);
};
