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
  // Refreshes replay-only UI state.
  void updateVisualState();
  // Resets the replay to the initial state.
  void reset();  // reset replay to beginning
  // Returns true if the current turn index is valid.
  bool currentTurnValid() const;
  // Returns true if a backward step is available.
  bool hasPreviousAction() const;
  // Steps one replay action forward. Returns true on success.
  bool stepForward();   // applies next action, false if none available
  // Steps one replay action backward. Returns true on success.
  bool stepBackward();  // undo last action, false if none available
  // Rebuilds replay state up to the target point.
  void replayToState(
      size_t targetTurnIndex,
      TurnPhase targetPhase);  // replay from initial state to given state
  // Applies the current turn's move action.
  void applyCurrentMove();
  // Applies the current turn's break action.
  void applyCurrentBreak();

  // Toggles replay autoplay on or off.
  void toggleAutoPlay();
  // Returns the autoplay delay for the current turn.
  long calculateAutoPlayDelay() const;  // calculate delay based on current
                                        // turn's think times and playback speed

 public:
  // Creates a replay session from saved replay data.
  explicit ReplaySession(const ReplayData& data);

  // Returns the display name for side.
  const std::string& playerName(Side side) const;

  void postUiMessage(const std::string& msg) {
    if (m_uiMessages.size() >= 256) {
      m_uiMessages.erase(m_uiMessages.begin());
    }
    m_uiMessages.push_back(msg);
  }
  const std::vector<std::string>& uiMessages() const { return m_uiMessages; }

  // updated new methods (similar to MatchSession)
  // Advances the replay by one frame of input.
  void update(int inputChar);
  // Returns the current replay state snapshot.
  const GameState& state() const;
  // Returns the current replay phase.
  TurnPhase phase() const;
  // Returns the stored replay turn history.
  const std::vector<TurnRecord>& history() const;
  // Returns replay-only UI state for rendering.
  const ReplayVisualState& visualState() const;
  // Returns true if replay state is terminal.
  bool isFinished() const;

  // auto-play methods
  // Enables or disables autoplay.
  void setAutoPlay(bool active);
  // Sets the fixed autoplay delay in ticks.
  void setAutoPlayDelay(int ticks);
  // Returns true if autoplay is active.
  bool isAutoPlayActive() const;
  // Sets the replay playback speed multiplier.
  void setPlaybackSpeed(float speed);
};
