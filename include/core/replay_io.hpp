#pragma once
#include <optional>
#include <string>

#include "replay_data.hpp"

class ReplayIO {
 public:
  static bool saveReplay(const ReplayData& data,
                         const std::string& outFilename = "");
  static std::optional<ReplayData> loadReplay(const std::string& filepath);

 private:
  static const std::string REPLAY_DIR;
  static bool ensureDirectoryExists();
  static std::string generateFilename(const std::string& name = "");
};
