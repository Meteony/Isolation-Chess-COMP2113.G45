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

Side oppositeSide(Side side) {
  return (side == Side::Player1) ? Side::Player2 : Side::Player1;
}

int randomThinkTicks(std::mt19937& rng) {
  // Main loop ticks at ~100ms, so 3-10 ticks approximates 300-1000ms.
  return std::uniform_int_distribution<int>(3, 10)(rng);
}

int minimaxDepth(const GameState& state, AiDifficulty difficulty) {
  if (difficulty != AiDifficulty::Hard) {
    return 3;
  }

  int totalTiles = state.rows() * state.cols();
  int intactTiles = countIntactTiles(state);
  bool isEndgame = (intactTiles * 100) < (totalTiles * 40);
  return isEndgame ? 4 : 3;
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

Coord AiPlayer::chooseAction(const GameState& state, bool isMove) {
  if (legalMoveCount(state, m_side) < 2) {
    return findMinimaxMove(state, isMove);
  }

  StrategyWeights weights = adjustedWeights(state, m_difficulty, m_turnCount);
  Strategy strategy = chooseStrategy(weights, m_rng);

  switch (strategy) {
    case Strategy::Random:
      return isMove ? findRandomMove(state) : findRandomBreak(state);
    case Strategy::Greedy:
      return findGreedyMove(state, isMove);
    case Strategy::Minimax:
      return findMinimaxMove(state, isMove);
  }

  return findMinimaxMove(state, isMove);
}

Coord AiPlayer::findRandomMove(const GameState& state) {
  auto moves = getLegalMoves(state, m_side);
  if (moves.empty()) return state.playerPos(m_side);
  std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
  return moves[dist(m_rng)];
}

Coord AiPlayer::findRandomBreak(const GameState& state) {
  auto breaks = getLegalBreaks(state);
  if (breaks.empty()) return {0, 0};
  std::uniform_int_distribution<size_t> dist(0, breaks.size() - 1);
  return breaks[dist(m_rng)];
}

Coord AiPlayer::findGreedyMove(const GameState& state, bool isMove) {
  if (isMove) {
    auto moves = getLegalMoves(state, m_side);
    Coord best = moves.empty() ? state.playerPos(m_side) : moves[0];
    int maxMobility = -1;
    for (auto& m : moves) {
      GameState sim = state;
      sim.setPlayerPos(m_side, m);
      int mobility = getLegalMoves(sim, m_side).size();
      if (mobility > maxMobility) {
        maxMobility = mobility;
        best = m;
      }
    }
    return best;
  } else {
    auto breaks = getLegalBreaks(state);
    if (breaks.empty()) return {0, 0};

    Coord best = breaks[0];
    int bestOppMobility = INT_MAX;
    int bestSelfMobility = -1;
    Side opp = oppositeSide(m_side);
    for (const auto& tile : breaks) {
      GameState sim = state;
      sim.setTile(tile, TileState::Broken);
      int oppMobility = legalMoveCount(sim, opp);
      int selfMobility = legalMoveCount(sim, m_side);
      if (oppMobility < bestOppMobility ||
          (oppMobility == bestOppMobility && selfMobility > bestSelfMobility)) {
        bestOppMobility = oppMobility;
        bestSelfMobility = selfMobility;
        best = tile;
      }
    }
    return best;
  }
}

int AiPlayer::evaluate(const GameState& state) {
  int myMoves = getLegalMoves(state, m_side).size();
  Side opponent = (m_side == Side::Player1) ? Side::Player2 : Side::Player1;
  int oppMoves = getLegalMoves(state, opponent).size();
  return myMoves - oppMoves;
}

int AiPlayer::minimax(GameState state, int depth, Side actor, TurnPhase phase,
                      int alpha, int beta) {
  if (depth == 0) {
    return evaluate(state);
  }

  bool isMaximizing = (actor == m_side);

  if (phase == TurnPhase::Move) {
    auto moves = getLegalMoves(state, actor);
    if (moves.empty()) {
      // If the actor cannot move, that side loses immediately.
      return isMaximizing ? -10000 : 10000;
    }

    if (isMaximizing) {
      int maxEval = INT_MIN;
      for (const auto& m : moves) {
        GameState sim = state;
        sim.setPlayerPos(actor, m);
        int eval =
            minimax(sim, depth - 1, actor, TurnPhase::Break, alpha, beta);
        maxEval = std::max(maxEval, eval);
        alpha = std::max(alpha, eval);
        if (beta <= alpha) break;
      }
      return maxEval;
    }

    int minEval = INT_MAX;
    for (const auto& m : moves) {
      GameState sim = state;
      sim.setPlayerPos(actor, m);
      int eval = minimax(sim, depth - 1, actor, TurnPhase::Break, alpha, beta);
      minEval = std::min(minEval, eval);
      beta = std::min(beta, eval);
      if (beta <= alpha) break;
    }
    return minEval;
  }

  auto breaks = getLegalBreaks(state);
  if (breaks.empty()) {
    return evaluate(state);
  }

  Side nextActor = oppositeSide(actor);
  if (isMaximizing) {
    int maxEval = INT_MIN;
    for (const auto& tile : breaks) {
      GameState sim = state;
      sim.setTile(tile, TileState::Broken);
      int eval =
          minimax(sim, depth - 1, nextActor, TurnPhase::Move, alpha, beta);
      maxEval = std::max(maxEval, eval);
      alpha = std::max(alpha, eval);
      if (beta <= alpha) break;
    }
    return maxEval;
  }

  int minEval = INT_MAX;
  for (const auto& tile : breaks) {
    GameState sim = state;
    sim.setTile(tile, TileState::Broken);
    int eval = minimax(sim, depth - 1, nextActor, TurnPhase::Move, alpha, beta);
    minEval = std::min(minEval, eval);
    beta = std::min(beta, eval);
    if (beta <= alpha) break;
  }
  return minEval;
}

Coord AiPlayer::findMinimaxMove(const GameState& state, bool isMove) {
  Side opp = oppositeSide(m_side);
  int searchDepth = minimaxDepth(state, m_difficulty);

  if (!isMove) {
    auto breaks = getLegalBreaks(state);
    Coord bestBreak = breaks.empty() ? Coord{0, 0} : breaks[0];
    int bestVal = INT_MIN;

    for (const auto& tile : breaks) {
      GameState sim = state;
      sim.setTile(tile, TileState::Broken);
      int breakVal =
          minimax(sim, searchDepth, opp, TurnPhase::Move, INT_MIN, INT_MAX);
      if (breakVal > bestVal) {
        bestVal = breakVal;
        bestBreak = tile;
      }
    }
    return bestBreak;
  }

  auto moves = getLegalMoves(state, m_side);
  Coord bestMove = moves.empty() ? state.playerPos(m_side) : moves[0];
  int bestVal = INT_MIN;

  for (const auto& m : moves) {
    GameState sim = state;
    sim.setPlayerPos(m_side, m);
    int moveVal =
        minimax(sim, searchDepth, m_side, TurnPhase::Break, INT_MIN, INT_MAX);
    if (moveVal > bestVal) {
      bestVal = moveVal;
      bestMove = m;
    }
  }
  return bestMove;
}
