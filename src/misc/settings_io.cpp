#include "misc/settings_io.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

/*literally .strip(). CPP should just come with this built in*/
std::string trimCopy(const std::string& s) {
  std::size_t start = 0;
  while (start < s.size() &&
         std::isspace(static_cast<unsigned char>(s[start]))) {
    ++start;
  }

  std::size_t end = s.size();
  while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
    --end;
  }

  return s.substr(start, end - start);
}

bool parseIntStrict(const std::string& s, int& outValue) {
  try {
    std::size_t pos = 0;
    int value = std::stoi(s, &pos);
    if (pos != s.size()) {
      return false;
    }
    outValue = value;
    return true;
  } catch (...) {
    return false;
  }
}

bool isValidIpv4(const std::string& ip) {
  if (ip.empty()) {
    return false;
  }

  std::vector<int> parts;
  std::string current;

  for (char ch : ip) {
    if (ch == '.') {
      if (current.empty()) {
        return false;
      }

      int value = 0;
      if (!parseIntStrict(current, value)) {
        return false;
      }

      if (value < 0 || value > 255) {
        return false;
      }

      parts.push_back(value);
      current.clear();
      continue;
    }

    if (!std::isdigit(static_cast<unsigned char>(ch))) {
      return false;
    }

    current.push_back(ch);
  }

  if (current.empty()) {
    return false;
  }

  int value = 0;
  if (!parseIntStrict(current, value)) {
    return false;
  }

  if (value < 0 || value > 255) {
    return false;
  }

  parts.push_back(value);

  return parts.size() == 4;
}

/*Wait so I can play locally right*/
bool isValidServerIp(const std::string& ip) {
  if (ip == "localhost") {
    return true;
  }

  return isValidIpv4(ip);
}

bool isValidServerPort(int port) { return port >= 1 && port <= 65535; }

bool isValidGameTagChar(char ch) {
  const unsigned char uch = static_cast<unsigned char>(ch);
  return std::isalnum(uch) || ch == '_' || ch == '-' || ch == '.';
}

bool isValidGameTag(const std::string& tag) {
  if (tag.empty() || tag.size() > 24) {
    return false;
  }

  for (char ch : tag) {
    if (!isValidGameTagChar(ch)) {
      return false;
    }
  }

  return true;
}

bool isValidSettings(const Settings& settings) {
  return isValidServerIp(settings.serverIp) &&
         isValidServerPort(settings.serverPort) &&
         isValidGameTag(settings.gameTag);
}

}  // namespace

std::string SettingsIO::defaultPath() { return "settings.cfg"; }

std::optional<Settings> SettingsIO::loadSettings(const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    return std::nullopt;
  }

  Settings settings;
  bool sawServerIp = false;
  bool sawServerPort = false;
  bool sawGameTag = false;

  std::string line;
  while (std::getline(file, line)) {
    line = trimCopy(line);

    if (line.empty()) {
      continue;
    }

    if (!line.empty() && line[0] == '#') {
      continue;
    }

    std::size_t eqPos = line.find('=');
    if (eqPos == std::string::npos) {
      return std::nullopt;
    }

    std::string key = trimCopy(line.substr(0, eqPos));
    std::string value = trimCopy(line.substr(eqPos + 1));

    if (key == "server_ip") {
      if (sawServerIp || !isValidServerIp(value)) {
        return std::nullopt;
      }
      settings.serverIp = value;
      sawServerIp = true;
      continue;
    }

    if (key == "server_port") {
      if (sawServerPort) {
        return std::nullopt;
      }

      int port = 0;
      if (!parseIntStrict(value, port) || !isValidServerPort(port)) {
        return std::nullopt;
      }

      settings.serverPort = port;
      sawServerPort = true;
      continue;
    }

    if (key == "game_tag") {
      if (sawGameTag || !isValidGameTag(value)) {
        return std::nullopt;
      }
      settings.gameTag = value;
      sawGameTag = true;
      continue;
    }

    return std::nullopt;
  }

  if (!sawServerIp || !sawServerPort || !sawGameTag) {
    return std::nullopt;
  }

  if (!isValidSettings(settings)) {
    return std::nullopt;
  }

  return settings;
}

bool SettingsIO::saveSettings(const Settings& settings,
                              const std::string& filepath) {
  if (!isValidSettings(settings)) {
    return false;
  }

  std::ofstream file(filepath);
  if (!file.is_open()) {
    return false;
  }

  file << "server_ip=" << settings.serverIp << "\n";
  file << "server_port=" << settings.serverPort << "\n";
  file << "game_tag=" << settings.gameTag << "\n";

  return true;
}
