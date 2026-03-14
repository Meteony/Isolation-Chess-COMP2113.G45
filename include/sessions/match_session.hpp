#pragma once
#include "coord.hpp"
#include "enums.hpp"
#include "game_state.hpp"
#include "player.hpp"
#include "turn_record.hpp"

class MatchSession {
private:
    GameState m_state;
    Player* m_p1;
    Player* m_p2;

    TurnPhase m_phase = TurnPhase::Move;
    bool m_hasPendingMove = false;
    Coord m_pendingMove{};

    TurnRecord* m_history = nullptr;
    int m_historySize = 0;
    int m_historyCapacity = 0;

private:
    Player* currentPlayer();
    const Player* currentPlayer() const;

    void beginTurn();
    void ensureHistoryCapacity();
    void pushHistory(const TurnRecord& record);

public:
    MatchSession(int rows, int cols, Player* p1, Player* p2);
    MatchSession(const MatchSession&) = delete;
    MatchSession& operator=(const MatchSession&) = delete;
    ~MatchSession();

    void handleInput(int ch);
    void update();

    const GameState& state() const;
    TurnPhase phase() const;

    const TurnRecord* history() const;
    int historySize() const;

    bool isFinished() const;
};
