#pragma once
#include <string>
#include <vector>

/*
Note to others: How this is different from game_state:
game_state packages only the bare minimum for
AI to quickly copy & calcluate the best path

match_session is way broader & encapsulates everything,
including even the gamer tag & full replay cache.
AI does't need any of those.
*/

#include "core/game_state.hpp"
#include "core/replay_data.hpp"
#include "core/turn_record.hpp"
#include "players/player.hpp"

// Misc HUD elements that aren't in GameState by themselves
struct MatchVisualState {
  bool cursorVisible = false;
  Coord cursor{};
};

class MatchSession {
 private:
  std::vector<std::string> m_uiMessages{};

  GameState m_initialState;
  GameState m_state;

  MatchVisualState m_visualState;

  Player* m_p1;
  Player* m_p2;

  std::string m_player1Name{"Player 1"};
  std::string m_player2Name{"Player 2"};

  int m_gameTick = 0;

  std::vector<TurnRecord> m_history;
  TurnRecord m_currentTurnRecord{};

 private:
  // Returns the player whose turn is active.
  Player& currentPlayer();
  // Returns the active player for const access.
  const Player& currentPlayer() const;

  // Appends one turn record to the history list.
  void pushHistory(const TurnRecord& record);

 public:
  // Creates a live match with board size, players, and names.
  MatchSession(int rows, int cols, Player* p1, Player* p2,
               std::string player1Name = "Player 1",
               std::string player2Name = "Player 2");

  MatchSession(const MatchSession&) = delete;
  MatchSession& operator=(const MatchSession&) = delete;
  ~MatchSession();

  // Advances the live match by one frame of input.
  void update(int inputChar);

  void postUiMessage(const std::string& msg) {
    if (m_uiMessages.size() >= 256) {
      m_uiMessages.erase(m_uiMessages.begin());
    }
    m_uiMessages.push_back(msg);
  }
  const std::vector<std::string>& uiMessages() const { return m_uiMessages; }

  // Returns the display name for side.
  const std::string& playerName(Side side) const;

  // Returns the current authoritative game state.
  const GameState& state() const;
  // Returns the current turn phase.
  TurnPhase phase() const;

  int gameTick() const { return m_gameTick; }

  // Builds replay data from the current match.
  ReplayData buildReplayData() const;

  // Returns UI-only visual state for rendering.
  const MatchVisualState& visualState() const;

  // Returns true if the match has finished.
  bool isFinished() const;
};
