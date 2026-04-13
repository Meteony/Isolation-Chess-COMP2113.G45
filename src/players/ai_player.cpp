#include "players/ai_player.hpp"
#include <algorithm>
#include <climits>
#include <ctime>
#include <random>

// get legal move.break list for a given state(can be in game rules or here, pending))
static std::vector<Coord> getLegalMoves(const GameState& state, Side side) {
    std::vector<Coord> moves;
    Coord pos = state.playerPos(side);
    for (int r = pos.row - 1; r <= pos.row + 1; ++r) {
        for (int c = pos.col - 1; c <= pos.col + 1; ++c) {
            Coord target{r, c};
            if (state.inBounds(target) && state.tileAt(target) == TileState::Intact) {
                if (target != state.playerPos(Side::Player1) && target != state.playerPos(Side::Player2)) {
                    moves.push_back(target);
                }
            }
        }
    }
    return moves;
}

static std::vector<Coord> getLegalBreaks(const GameState& state) {
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

AiPlayer::AiPlayer(AiDifficulty difficulty, Side side)
    : m_difficulty(difficulty), m_side(side), m_moveReady(false), m_breakReady(false), m_ticksUntilReady(0) {
    std::srand(std::time(nullptr));
}

void AiPlayer::beginMovePhase(const GameState& state) {
    m_moveReady = false;
    m_ticksUntilReady = 6 + (std::rand() % 9); // 6..14
}


void AiPlayer::beginBreakPhase(const GameState& state) {
    m_breakReady = false;
    m_ticksUntilReady = 6 + (std::rand() % 9); // 6..14
}

void AiPlayer::update(int ch, const GameState& state) {
    if (m_ticksUntilReady > 0) {
        --m_ticksUntilReady;
        return;
    }
    
    if (state.phase() == TurnPhase::Move && !m_moveReady) {
        if (m_difficulty == AiDifficulty::Easy) m_nextMove = findRandomMove(state);
        else if (m_difficulty == AiDifficulty::Medium) m_nextMove = findGreedyMove(state, true);
        else m_nextMove = findMinimaxMove(state, true);
        m_moveReady = true;
    } 
    else if (state.phase() == TurnPhase::Break && !m_breakReady) {
        if (m_difficulty == AiDifficulty::Easy) m_nextBreak = findRandomBreak(state);
        else if (m_difficulty == AiDifficulty::Medium) m_nextBreak = findGreedyMove(state, false);
        else m_nextBreak = findMinimaxMove(state, false);
        m_breakReady = true;
    }
}

//Easy: random valid move/break
Coord AiPlayer::findRandomMove(const GameState& state) {
    auto moves = getLegalMoves(state, m_side);
    if (moves.empty()) return state.playerPos(m_side);
    return moves[std::rand() % moves.size()];
}

Coord AiPlayer::findRandomBreak(const GameState& state) {
    auto breaks = getLegalBreaks(state);
    if (breaks.empty()) return {0, 0};
    return breaks[std::rand() % breaks.size()];
}

// Medium: greedy
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
        return findRandomBreak(state); 
    }
}

//Hard (Minimax + Alpha-Beta Pruning)
int AiPlayer::evaluate(const GameState& state) {
    int myMoves = getLegalMoves(state, m_side).size();
    Side opponent = (m_side == Side::Player1) ? Side::Player2 : Side::Player1;
    int oppMoves = getLegalMoves(state, opponent).size();
    return myMoves - oppMoves;
}

int AiPlayer::minimax(GameState state, int depth, bool isMaximizing, int alpha, int beta) {
    if (depth == 0 || getLegalMoves(state, Side::Player1).empty() || getLegalMoves(state, Side::Player2).empty()) {
        return evaluate(state);
    }

    if (isMaximizing) {
        int maxEval = INT_MIN;
        for (auto& m : getLegalMoves(state, m_side)) {
            GameState sim = state;
            sim.setPlayerPos(m_side, m);
            int eval = minimax(sim, depth - 1, false, alpha, beta);
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break;
        }
        return maxEval;
    } else {
        int minEval = INT_MAX;
        Side opp = (m_side == Side::Player1) ? Side::Player2 : Side::Player1;
        for (auto& m : getLegalMoves(state, opp)) {
            GameState sim = state;
            sim.setPlayerPos(opp, m);
            int eval = minimax(sim, depth - 1, true, alpha, beta);
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) break;
        }
        return minEval;
    }
}  // <-- this was the missing brace

Coord AiPlayer::findMinimaxMove(const GameState& state, bool isMove) {
    if (!isMove) return findRandomBreak(state);

    auto moves = getLegalMoves(state, m_side);
    Coord bestMove = moves.empty() ? state.playerPos(m_side) : moves[0];
    int bestVal = INT_MIN;

    for (auto& m : moves) {
        GameState sim = state;
        sim.setPlayerPos(m_side, m);
        int moveVal = minimax(sim, 3, false, INT_MIN, INT_MAX);
        if (moveVal > bestVal) {
            bestVal = moveVal;
            bestMove = m;
        }
    }
    return bestMove;
}
