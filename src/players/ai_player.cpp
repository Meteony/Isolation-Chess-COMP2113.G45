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
    : m_difficulty(difficulty), m_side(side), m_moveReady(false), m_breakReady(false) {
    std::srand(std::time(nullptr));
}

void AiPlayer::beginMovePhase(const GameState& state) {
    m_moveReady = false;
}

void AiPlayer::beginBreakPhase(const GameState& state) {
    m_breakReady = false;
}

void AiPlayer::update(int ch, const GameState& state) {
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
        return findRandomBreak(state); // 贪心难度下，破坏动作仍可保持部分随机以增加趣味
    }
}

//Hard (Minimax + Alpha-Beta Pruning)
