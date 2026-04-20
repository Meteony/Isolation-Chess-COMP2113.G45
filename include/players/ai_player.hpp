#pragma once
#include <random>

#include "core/enums.hpp"
#include "player.hpp"

class AiPlayer : public Player {
 public:
  // Creates an AI player for the given difficulty and side.
  AiPlayer(AiDifficulty difficulty, Side side);
  ~AiPlayer() override = default;

  // Starts AI move planning from state.
  void beginMovePhase(const GameState&) override;
  // Starts AI break planning from state.
  void beginBreakPhase(const GameState&) override;

  // Advances AI thinking using the latest state.
  void update(int, const GameState& state) override;

  // Picks a random legal move from state.
  Coord findRandomMove(const GameState& state);

  // Picks a random legal break from state.
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

  // Picks a greedy move or break from state.
  Coord findGreedyMove(const GameState& state, bool isMove);
  // Picks a minimax move or break from state.
  Coord findMinimaxMove(const GameState& state, bool isMove);
  // Chooses one action from the current strategy mix.
  Coord chooseAction(const GameState& state, bool isMove);

  // Scores state from this AI player's perspective.
  int evaluate(const GameState& state);
  // Searches future states and returns the best score.
  int minimax(GameState state, int depth, Side actor, TurnPhase phase,
              int alpha, int beta);
};
