#pragma once
#include <vector>
#include "coord.hpp"
#include "enums.hpp"

class GameState {
private:
    int m_rows;
    int m_cols;
    std::vector<TileState> m_tiles;

    Coord m_p1Pos;
    Coord m_p2Pos;

    Side m_sideToMove;
    TurnPhase m_phase;
    SessionStatus m_status;
    Side m_winner;

public:
    GameState(int rows, int cols);

    GameState(const GameState& other) = default;
    GameState& operator=(const GameState& other) = default;
    ~GameState() = default;

    int rows() const;
    int cols() const;

    bool inBounds(Coord c) const;
    int index(Coord c) const;

    TileState tileAt(Coord c) const;
    void setTile(Coord c, TileState state);

    Coord playerPos(Side side) const;
    void setPlayerPos(Side side, Coord pos);

    Side sideToMove() const;
    void setSideToMove(Side side);

    TurnPhase phase() const;
    void setPhase(TurnPhase phase);

    SessionStatus status() const;
    void setStatus(SessionStatus status);

    Side winner() const;
    void setWinner(Side side);
};
