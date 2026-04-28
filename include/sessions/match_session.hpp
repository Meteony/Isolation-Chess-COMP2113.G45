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

#include "core/enums.hpp"
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
 public:
  // Standard local-game factories. MatchSession creates and owns the players,
  // so callers no longer need to allocate the usual HumanPlayer/AiPlayer pair.
  static MatchSession CreateHumanVsHuman(
      int rows, int cols, std::string player1Name = "Player 1",
      std::string player2Name = "Player 2");
  static MatchSession CreateHumanVsAi(
      int rows, int cols, AiDifficulty difficulty,
      std::string humanName = "Player 1", std::string aiName = "CPU");
  static MatchSession CreateAiVsAi(
      int rows, int cols, AiDifficulty player1Difficulty,
      AiDifficulty player2Difficulty, std::string player1Name = "CPU 1",
      std::string player2Name = "CPU 2");

  // Escape hatch for netplay/tests/custom players. Ownership is transferred to
  // MatchSession; do not delete the pointers after passing them here.
  static MatchSession TakeOwnership(
      int rows, int cols, Player* p1, Player* p2,
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

 private:
  // Private on purpose: normal callers should use the named factories above so
  // the player ownership model is obvious at the call site.
  MatchSession(int rows, int cols, Player* p1, Player* p2,
               std::string player1Name = "Player 1",
               std::string player2Name = "Player 2");

  // Returns the player whose turn is active.
  Player& currentPlayer();
  // Returns the active player for const access.
  const Player& currentPlayer() const;

  // Appends one turn record to the history list.
  void pushHistory(const TurnRecord& record);

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
};
