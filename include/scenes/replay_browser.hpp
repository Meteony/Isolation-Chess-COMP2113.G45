#pragma once

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "core/replay_io.hpp"

namespace scenes {

struct ReplaySummary {
  std::string path;
  std::string fileName;
  std::string winner;
  std::string player1Name;
  std::string player2Name;
  int turns = 0;
  int rows = 0;
  int cols = 0;
};

inline std::string replayWinnerLabel(const ReplayData& replay) {
  if (replay.winner == 0) {
    return replay.player1Name.empty() ? "Player 1" : replay.player1Name;
  }
  if (replay.winner == 1) {
    return replay.player2Name.empty() ? "Player 2" : replay.player2Name;
  }
  return "Unknown";
}

inline std::vector<ReplaySummary> scanReplays() {
  namespace fs = std::filesystem;

  std::vector<ReplaySummary> out;
  const fs::path replayDir("replays");
  if (!fs::exists(replayDir) || !fs::is_directory(replayDir)) {
    return out;
  }

  for (const fs::directory_entry& entry : fs::directory_iterator(replayDir)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    const std::optional<ReplayData> replay =
        ReplayIO::loadReplay(entry.path().string());
    if (!replay) {
      continue;
    }

    ReplaySummary summary;
    summary.path = entry.path().string();
    summary.fileName = entry.path().filename().string();
    summary.winner = replayWinnerLabel(*replay);
    summary.player1Name =
        replay->player1Name.empty() ? "Player 1" : replay->player1Name;
    summary.player2Name =
        replay->player2Name.empty() ? "Player 2" : replay->player2Name;
    summary.turns = static_cast<int>(replay->history.size());
    summary.rows = replay->initialState.rows();
    summary.cols = replay->initialState.cols();
    out.push_back(summary);
  }

  std::sort(out.begin(), out.end(),
            [](const ReplaySummary& a, const ReplaySummary& b) {
              return a.fileName < b.fileName;
            });
  return out;
}

}  // namespace scenes
