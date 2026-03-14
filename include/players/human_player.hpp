#pragma once
#include "player.hpp"
#include "input_overlay.hpp"
#include "enums.hpp"

class HumanPlayer : public Player {
private:
    Coord m_cursor{};
    Coord m_pendingMove{};
    Coord m_pendingBreak{};

    bool m_moveReady = false;
    bool m_breakReady = false;
    bool m_hasSelectedMove = false;

    TurnPhase m_phase = TurnPhase::Move;

public:
    HumanPlayer();
    ~HumanPlayer() override = default;

    void beginMovePhase(const GameState& state) override;
    void beginBreakPhase(const GameState& state) override;

    void handleInput(int ch) override;
    void update(const GameState& state) override;

    bool hasMoveReady() const override;
    Coord consumeMove() override;

    bool hasBreakReady() const override;
    Coord consumeBreak() override;

    InputOverlay makeOverlay() const;
};
c
