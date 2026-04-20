#pragma once

/*
Example usage:

#include "misc/settings_io.hpp"

std::optional<Settings> settings = SettingsIO::loadSettings();
if (!settings) {
  // handle invalid / missing settings
  return;
}

const std::string& ip = settings->serverIp;
int port = settings->serverPort;
const std::string& tag = settings->gameTag;


Saving:

Settings settings;
settings.serverIp = "127.0.0.1";
settings.serverPort = 4567;
settings.gameTag = "John";

if (!SettingsIO::saveSettings(settings)) {
  // handle save failure
}

*/

#include <optional>
#include <string>

struct Settings {
  std::string serverIp = "localhost";
  int serverPort = 5050;
  std::string gameTag = "Player";
};

class SettingsIO {
 public:
  static std::optional<Settings> loadSettings(
      const std::string& filepath = defaultPath());

  static bool saveSettings(const Settings& settings,
                           const std::string& filepath = defaultPath());

  static std::string defaultPath();
};
