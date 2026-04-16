#include "players/ai_player.hpp"

#include <algorithm>
#include <climits>
#include <random>

namespace {

enum class Strategy {
  Random,
  Greedy,
  Minimax,
};

struct StrategyWeights {
  int random = 0;
  int greedy = 0;
  int minimax = 0;
};

std::vector<Coord> getLegalMoves(const GameState& state, Side side) {
  std::vector<Coord> moves;
  Coord pos = state.playerPos(side);
  for (int r = pos.row - 1; r <= pos.row + 1; ++r) {
    for (int c = pos.col - 1; c <= pos.col + 1; ++c) {
      Coord target{r, c};
      if (state.inBounds(target) && state.tileAt(target) == TileState::Intact) {
        if (target != state.playerPos(Side::Player1) &&
            target != state.playerPos(Side::Player2)) {
          moves.push_back(target);
        }
      }
    }
  }
  return moves;
}

std::vector<Coord> getLegalBreaks(const GameState& state) {
  std::vector<Coord> breaks;
  for (int r = 0; r < state.rows(); ++r) {
    for (int c = 0; c < state.cols(); ++c) {
      Coord target{r, c};
      if (state.tileAt(target) == TileState::Intact &&
          target != state.playerPos(Side::Player1) &&
          target != state.playerPos(Side::Player2)) {
        breaks.push_back(target);
      }
    }
  }
  return breaks;
}

int countIntactTiles(const GameState& state) {
  int intact = 0;
  for (int r = 0; r < state.rows(); ++r) {
    for (int c = 0; c < state.cols(); ++c) {
      if (state.tileAt({r, c}) == TileState::Intact) {
        ++intact;
      }
    }
  }
  return intact;
}

StrategyWeights baseWeights(AiDifficulty difficulty) {
  switch (difficulty) {
    case AiDifficulty::Easy:
      return {60, 30, 10};
    case AiDifficulty::Medium:
      return {20, 50, 30};
    case AiDifficulty::Hard:
      return {0, 20, 80};
  }
  return {0, 0, 100};
}

StrategyWeights adjustedWeights(const GameState& state, AiDifficulty difficulty,
                                int turnCount) {
  StrategyWeights weights = baseWeights(difficulty);

  if (difficulty == AiDifficulty::Medium) {
    int shifts = turnCount / 3;
    int shiftAmount = std::min(weights.random, shifts * 5);
    weights.random -= shiftAmount;
    weights.minimax += shiftAmount;
  }

  int totalTiles = state.rows() * state.cols();
  int intactTiles = countIntactTiles(state);
  bool isEndgame = (intactTiles * 100) < (totalTiles * 40);
  if ((difficulty == AiDifficulty::Medium ||
       difficulty == AiDifficulty::Hard) &&
      isEndgame) {
    weights.random = 0;
    weights.greedy = 0;
    weights.minimax = 100;
  }

  return weights;
}

Strategy chooseStrategy(const StrategyWeights& weights, std::mt19937& rng) {
  int total = weights.random + weights.greedy + weights.minimax;
  if (total <= 0) return Strategy::Minimax;

  std::uniform_int_distribution<int> dist(1, total);
  int roll = dist(rng);
  if (roll <= weights.random) return Strategy::Random;
  roll -= weights.random;
  if (roll <= weights.greedy) return Strategy::Greedy;
  return Strategy::Minimax;
}

int legalMoveCount(const GameState& state, Side side) {
  return static_cast<int>(getLegalMoves(state, side).size());
}

int randomThinkTicks(std::mt19937& rng) {
  // Main loop ticks at ~100ms, so 3-10 ticks approximates 300-1000ms.
  return std::uniform_int_distribution<int>(3, 10)(rng);
}

}  // namespace

AiPlayer::AiPlayer(AiDifficulty difficulty, Side side)
    : m_difficulty(difficulty),
      m_side(side),
      m_moveReady(false),
      m_breakReady(false),
      m_ticksUntilReady(0),
      m_turnCount(0),
      m_rng(std::random_device{}()) {}

void AiPlayer::beginMovePhase(const GameState& state) {
  (void)state;
  m_moveReady = false;
  m_ticksUntilReady = randomThinkTicks(m_rng);
  ++m_turnCount;
}

void AiPlayer::beginBreakPhase(const GameState& state) {
  (void)state;
  m_breakReady = false;
  m_ticksUntilReady = randomThinkTicks(m_rng);
}

void AiPlayer::update(int ch, const GameState& state) {
  (void)ch;
  if (m_ticksUntilReady > 0) {
    --m_ticksUntilReady;
    return;
  }

  if (state.phase() == TurnPhase::Move && !m_moveReady) {
    m_nextMove = chooseAction(state, true);
    m_moveReady = true;
  } else if (state.phase() == TurnPhase::Break && !m_breakReady) {
    m_nextBreak = chooseAction(state, false);
    m_breakReady = true;
  }
}

