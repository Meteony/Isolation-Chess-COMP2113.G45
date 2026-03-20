#pragma once
#include <vector>
#include "game_state.hpp"
#include "player.hpp"
#include "turn_record.hpp"

// Misc HUD elements that aren't in GameState by themselves
struct MatchVisualState {
    bool cursorVisible = false;
    Coord cursor{};
};

class MatchSession {
private:
    GameState m_state;
    Player* m_p1;
    Player* m_p2;

    std::vector<TurnRecord> m_history;
    TurnRecord m_currentTurnRecord{};

    MatchVisualState m_visualState;

    int m_gameTick = 0;

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

    const std::vector<TurnRecord>& history() const;

    const MatchVisualState& visualState() const;

    bool isFinished() const;
};
