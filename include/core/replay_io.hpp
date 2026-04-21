#pragma once
#include <optional>
#include <string>

#include "replay_data.hpp"

class ReplayIO {
 public:
  // Saves data to a replay file. Returns true on success.
  static bool saveReplay(const ReplayData& data,
                         const std::string& outFilename = "");
  // Loads a replay from filepath. Returns parsed data or nullopt.
  static std::optional<ReplayData> loadReplay(const std::string& filepath);
  static std::optional<ReplayMetadata> loadReplayMetadata(
      const std::string& filepath);

 private:
  static const std::string REPLAY_DIR;
  // Creates the replay folder if needed. Returns true on success.
  static bool ensureDirectoryExists();
  // Builds a replay file path from name. Returns the final path.
  static std::string generateFilename(const std::string& name = "");
};
