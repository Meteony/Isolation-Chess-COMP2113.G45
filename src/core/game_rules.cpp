#include "core/game_rules.hpp"
#include <cmath>

bool GameRules::isLegalMove(const GameState& state, Side side, Coord dst)
{
    //dst within board bounds
    if (!state.inBounds(dst))
        return false;
    
    //dst intact
    if (state.tileAt(dst) != TileState::Intact)
        return false;
    
    //dst not be occupied by any player
    if (dst == state.playerPos(Side::Player1))
        return false;
    if (dst == state.playerPos(Side::Player2))
        return false;
    
    //dst within 3x3 range of current player position
    Coord currentPos = state.playerPos(side);
    int rowDiff = std::abs(dst.row - currentPos.row);
    int colDiff = std::abs(dst.col - currentPos.col);
    if (rowDiff > 1 || colDiff > 1)
        return false;
    
    return true;
}


bool GameRules::isLegalBreak(const GameState& state, Coord tile)
{
    //tile within board bounds
    if (!state.inBounds(tile))
        return false;
    
    //tile intact
    if (state.tileAt(tile) != TileState::Intact)
        return false;
    
    //tile not be occupied by any player
    if (tile == state.playerPos(Side::Player1))
        return false;
    if (tile == state.playerPos(Side::Player2))
        return false;

    return true;
}


void GameRules::applyMove(GameState& state, Side side, Coord dst)
{
    state.setPlayerPos(side, dst);
}


void GameRules::applyBreak(GameState& state, Coord tile)
{
    state.setTile(tile, TileState::Broken);
}


bool GameRules::hasAnyLegalMove(const GameState& state, Side side)
{
    Coord currentPos = state.playerPos(side);
    
    for (int r=currentPos.row-1; r<=currentPos.row+1; r++)
    {
        for (int c=currentPos.col-1; c <=currentPos.col+1; c++)
        {
            Coord target{r,c};
            if (isLegalMove(state, side, target))
                return true;
        }
    }

    return false;
}


bool GameRules::isTerminal(const GameState& state)
{
    Side currentPlayer = state.sideToMove();
    
    if (!hasAnyLegalMove(state, currentPlayer))
        return true;

    return false;
}
