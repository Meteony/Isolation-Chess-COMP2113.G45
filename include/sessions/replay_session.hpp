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
  // Helper methods.

  // Purpose: Refresh replay-only UI fields from the current replay state.
  // Input: none.
  // Output: none (updates internal visual state cache).
  void updateVisualState();

  // Purpose: Reset replay to the initial state and start position.
  // Input: none.
  // Output: none (resets internal replay state/index data).
  void reset();

  // Purpose: Check whether the current turn is valid.
  // Input: none.
  // Output: true if the replay is not finished and the current turn index is
  // within history; otherwise false.
  bool currentTurnValid() const;

  // Purpose: Check whether one backward replay action is currently available.
  // Input: none.
  // Output: true if at least one reverse step is possible from the current
  // turn/phase (including finished-state rewind); otherwise false.
  bool hasPreviousAction() const;

  // Purpose: Advance replay by one action from the current position.
  // Input: none.
  // Output: true if a next action existed and was applied; otherwise false.
  bool stepForward();

  // Purpose: Move replay back by one action from the current position.
  // Input: none.
  // Output: true if a previous action existed and was returned to; otherwise
  // false.
  bool stepBackward();

  // Purpose: Rebuild replay state from initial state to a target turn/phase.
  // Input: targetTurnIndex (0-based turn index), targetPhase (phase in target
  // turn). Output: none (mutates internal replay state to requested target).
  void replayToState(size_t targetTurnIndex, TurnPhase targetPhase);

  // Purpose: Apply the move action for the current turn record.
  // Input: none.
  // Output: none (updates replay state according to current move action).
  void applyCurrentMove();

  // Purpose: Apply the break action for the current turn record.
  // Input: none.
  // Output: none (updates replay state according to current break action).
  void applyCurrentBreak();

  // Purpose: Switch autoplay mode between enabled and disabled.
  // Input: none.
  // Output: none (toggles internal autoplay active flag).
  void toggleAutoPlay();

  // Purpose: Compute autoplay delay for current turn using think time and
  // speed. Input: none. Output: delay in ticks used by autoplay timing.
  long calculateAutoPlayDelay() const;

 public:
  // Purpose: Construct a replay session from serialized replay data.
  // Input: data (replay metadata, initial state, and turn history).
  // Output: initialized ReplaySession object.
  explicit ReplaySession(const ReplayData& data);

  // Purpose: Jump replay to the start of a requested turn.
  // Input: turnNumber (1-based turn number).
  // Output: true if jump succeeds; otherwise false.
  bool goToTurn(size_t turnNumber);

  // Purpose: Get display name of the specified player side.
  // Input: side (Side::Player1 or Side::Player2).
  // Output: const reference to selected player's display name.
  const std::string& playerName(Side side) const;

  // Purpose: Append a UI message to the replay message log.
  // Input: msg (message text to append).
  // Output: none (log is updated; oldest item is dropped if size exceeds 256).
  void postUiMessage(const std::string& msg) {
    if (m_uiMessages.size() >= 256) {
      m_uiMessages.erase(m_uiMessages.begin());
    }
    m_uiMessages.push_back(msg);
  }

  // Purpose: Access all queued UI messages for rendering.
  // Input: none.
  // Output: const reference to internal UI message list.
  const std::vector<std::string>& uiMessages() const { return m_uiMessages; }

  // Purpose: Advance replay logic by one frame using current input.
  // Input: inputChar (user input code for replay controls).
  // Output: none (updates replay state, autoplay timers, and UI state).
  void update(int inputChar);

  // Purpose: Get current replay game-state snapshot.
  // Input: none.
  // Output: const reference to current GameState.
  const GameState& state() const;

  // Purpose: Get current replay phase within the turn flow.
  // Input: none.
  // Output: current TurnPhase value.
  TurnPhase phase() const;

  // Purpose: Access full replay turn history.
  // Input: none.
  // Output: const reference to vector of TurnRecord entries.
  const std::vector<TurnRecord>& history() const;

  // Purpose: Get replay-specific visual state for UI rendering.
  // Input: none.
  // Output: const reference to ReplayVisualState.
  const ReplayVisualState& visualState() const;

  // Purpose: Check whether replay has reached a terminal game state.
  // Input: none.
  // Output: true if game is finished; otherwise false.
  bool isFinished() const;

  // Purpose: Enable or disable replay autoplay mode.
  // Input: active (true to enable, false to disable).
  // Output: none (updates autoplay state and resets the autoplay counter).
  void setAutoPlay(bool active);

  // Purpose: Set the current autoplay target delay used by timer logic.
  // Input: ticks (delay length in ticks).
  // Output: none (updates the runtime autoplay target delay value; this value
  // may be recalculated by per-frame autoplay timing logic).
  void setAutoPlayDelay(int ticks);

  // Purpose: Query whether autoplay is currently enabled.
  // Input: none.
  // Output: true if autoplay is active; otherwise false.
  bool isAutoPlayActive() const;

  // Purpose: Set replay playback speed multiplier for timing.
  // Input: speed (multiplier where 1.0 is normal speed).
  // Output: none (updates internal playback speed).
  void setPlaybackSpeed(float speed);
};
