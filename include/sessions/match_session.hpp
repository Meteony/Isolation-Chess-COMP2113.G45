#pragma once
#include <vector>
#include "core/game_state.hpp"
#include "players/player.hpp"
#include "core/turn_record.hpp"
#include "core/replay_data.hpp"

// Misc HUD elements that aren't in GameState by themselves
struct MatchVisualState {
    bool cursorVisible = false;
    Coord cursor{};
};

class MatchSession {
private:
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

    const GameState& state() const;
    TurnPhase phase() const;

    ReplayData buildReplayData() const;

    const MatchVisualState& visualState() const;

    bool isFinished() const;
};
