#pragma once
#include <string>
#include <vector>
#include "game_state.hpp"
#include "turn_record.hpp"

class ReplayIO
{
public:
    struct ReplayData
    {
        int rows;
        int cols;
        GameState initialState;
        std::vector<TurnRecord> history;
    };

    static bool saveReplay(int rows,
                          int cols,
                          const GameState& initialState,
                          const std::vector<TurnRecord>& history,
                          std::string& outFilename);

    static bool loadReplay(const std::string& filepath,
                          ReplayData& outData);

private:
    static const std::string REPLAY_DIR;
    static bool ensureDirectoryExists();
    static std::string generateFilename();
};
