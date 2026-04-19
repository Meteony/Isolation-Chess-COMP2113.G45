#pragma once
#include <string>
#include <vector>

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

  int m_gameTick = 0;

  std::vector<TurnRecord> m_history;
  TurnRecord m_currentTurnRecord{};

 private:
  Player& currentPlayer();
  const Player& currentPlayer() const;

  void pushHistory(const TurnRecord& record);

 public:
  MatchSession(int rows, int cols, Player* p1, Player* p2);
  MatchSession(const MatchSession&) = delete;
  MatchSession& operator=(const MatchSession&) = delete;
  ~MatchSession();

  void update(int inputChar);

  void postUiMessage(const std::string& msg) {
    if (m_uiMessages.size() >= 256) {
      m_uiMessages.erase(m_uiMessages.begin());
    }
    m_uiMessages.push_back(msg);
  }
  const std::vector<std::string>& uiMessages() const { return m_uiMessages; }

  const GameState& state() const;
  TurnPhase phase() const;

  int gameTick() const { return m_gameTick; }

  ReplayData buildReplayData() const;

  const MatchVisualState& visualState() const;

  bool isFinished() const;
};
