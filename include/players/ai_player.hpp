#pragma once
#include <random>

#include "core/enums.hpp"
#include "player.hpp"

class AiPlayer : public Player {
 public:
  AiPlayer(AiDifficulty difficulty, Side side);
  ~AiPlayer() override = default;

  void beginMovePhase(const GameState&) override;
  void beginBreakPhase(const GameState&) override;

  void update(int, const GameState& state) override;

  Coord findRandomMove(const GameState& state);

  Coord findRandomBreak(const GameState& state);

  bool hasMoveReady() const override { return m_moveReady; }
  Coord consumeMove() override {
    m_moveReady = false;
    return m_nextMove;
  }

  bool hasBreakReady() const override { return m_breakReady; }
  Coord consumeBreak() override {
    m_breakReady = false;
    return m_nextBreak;
  }

 private:
  AiDifficulty m_difficulty;
  Side m_side;

  bool m_moveReady = false;
  bool m_breakReady = false;

  Coord m_nextMove;
  Coord m_nextBreak;

  int m_ticksUntilReady = 0;
  int m_turnCount = 0;

  std::mt19937 m_rng;

  Coord findGreedyMove(const GameState& state, bool isMove);
  Coord findMinimaxMove(const GameState& state, bool isMove);
  Coord chooseAction(const GameState& state, bool isMove);

  int evaluate(const GameState& state);
  int minimax(GameState state, int depth, Side actor, TurnPhase phase,
              int alpha, int beta);
};
