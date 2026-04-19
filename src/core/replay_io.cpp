#include "core/replay_io.hpp"

#include <cassert>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>

const std::string ReplayIO::REPLAY_DIR = "replays";

/*Validate legal player name*/
namespace {

constexpr std::size_t kMaxPlayerNameLength = 24;

bool isValidPlayerNameChar(char ch) {
  const unsigned char uch = static_cast<unsigned char>(ch);
  return std::isalnum(uch) || ch == ' ' || ch == '_' || ch == '-' || ch == '.';
}

bool isValidPlayerName(const std::string& name) {
  if (name.empty()) {
    return true;  // omitted is allowed
  }

  if (name.size() > kMaxPlayerNameLength) {
    return false;
  }

  if (name.front() == ' ' || name.back() == ' ') {
    return false;
  }

  for (char ch : name) {
    if (!isValidPlayerNameChar(ch)) {
      return false;
    }
  }

  return true;
}

}  // namespace

bool ReplayIO::ensureDirectoryExists() {
  try {
    if (!std::filesystem::exists(REPLAY_DIR)) {
      std::filesystem::create_directory(REPLAY_DIR);
    }
    return true;
  } catch (const std::exception& e) {
    return false;
  }
}

#include <ctime>
#include <filesystem>
#include <string>

/*Default argument is in header*/
std::string ReplayIO::generateFilename(const std::string& name) {
  namespace fs = std::filesystem;

  fs::path dir(REPLAY_DIR);
  std::string baseName;

  if (name.empty()) {
    std::time_t now = std::time(nullptr);
    std::tm timeinfo = *std::localtime(&now);

    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "replay_%Y%m%d_%H%M%S", &timeinfo);
    baseName = buffer;
  } else {
    baseName = name;
  }

  fs::path inputName(baseName);

  // If no extension was given, force .isor
  std::string stem = inputName.stem().string();
  std::string ext =
      inputName.has_extension() ? inputName.extension().string() : ".isor";

  fs::path candidate = dir / (stem + ext);

  int suffix = 1;
  while (fs::exists(candidate)) {
    candidate = dir / (stem + "_" + std::to_string(suffix) + ext);
    ++suffix;
  }

  return candidate.string();
}

bool ReplayIO::saveReplay(const ReplayData& data,
                          const std::string& outFilename) {
  const GameState& initialState = data.initialState;
  int rows = initialState.rows();
  int cols = initialState.cols();
  const std::vector<TurnRecord>& history = data.history;

  if (!isValidPlayerName(data.player1Name) ||
      !isValidPlayerName(data.player2Name)) {
    assert(false && "ReplayIO::saveReplay: Invalid player name");
    return false;
  }

  if (!ensureDirectoryExists()) {
    assert(false && "ReplayIO::saveReplay: Directory doesn't exist");
    return false;
  }

  std::ofstream file(generateFilename(outFilename));
  if (!file.is_open()) {
    assert(false && "ReplayIO::saveReplay: Failed to open file");
    return false;
  }

  // save all initial states needed to construct
  // the game from the ground up
  // good if we add custom maps in the future

  file << "HEADER\n";
  file << "rows=" << rows << "\n";
  file << "cols=" << cols << "\n";

  Coord p1Pos = initialState.playerPos(Side::Player1);
  file << "p1_row=" << p1Pos.row << "\n";
  file << "p1_col=" << p1Pos.col << "\n";

  Coord p2Pos = initialState.playerPos(Side::Player2);
  file << "p2_row=" << p2Pos.row << "\n";
  file << "p2_col=" << p2Pos.col << "\n";

  file << "side_to_move=" << static_cast<int>(initialState.sideToMove())
       << "\n";
  file << "phase=" << static_cast<int>(initialState.phase()) << "\n";
  file << "status=" << static_cast<int>(initialState.status()) << "\n";

  file << "winner=" << data.winner << "\n";

  if (!data.player1Name.empty()) {
    file << "player1_name=" << data.player1Name << "\n";
  }

  if (!data.player2Name.empty()) {
    file << "player2_name=" << data.player2Name << "\n";
  }

  file << "TILES\n";
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      Coord here{r, c};
      file << static_cast<int>(initialState.tileAt(here));
      if (c + 1 < cols) {
        file << ' ';
      }
    }
    file << '\n';
  }
  file << "END_TILES\n";

  file << "END_HEADER\n";

  file << "TURNS\n";
  for (const auto& turn : history) {
    const int actor = (turn.actor == Side::Player1) ? 0 : 1;

    file << actor << ' ' << turn.moveCoord.row << ' ' << turn.moveCoord.col
         << ' ' << turn.breakCoord.row << ' ' << turn.breakCoord.col << ' '
         << turn.thinkTicksBeforeMove << ' ' << turn.thinkTicksBeforeBreak
         << '\n';
  }
  file << "END_TURNS\n";

  file << "UI_MESSAGES\n";
  file << "count=" << data.uiMessages.size() << "\n";
  for (const auto& msg : data.uiMessages) {
    file << "msg=" << std::quoted(msg) << "\n";
  }
  file << "END_UI_MESSAGES\n";

  return true;
}

std::optional<ReplayData> ReplayIO::loadReplay(const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    return std::nullopt;
  }

  // Final replay history to return.
  std::vector<TurnRecord> history{};
  std::vector<std::string> uiMessages{};

  // Current line buffer while scanning the file.
  std::string line;

  // ---------- Header data (parsed in pass 1) ----------
  int rows = -1;
  int cols = -1;

  int p1Row = -1;
  int p1Col = -1;
  int p2Row = -1;
  int p2Col = -1;

  int sideToMoveRaw = 0;
  int phaseRaw = 0;
  int statusRaw = 0;
  int winnerRaw = -1;

  std::string player1Name;
  std::string player2Name;

  // Stores the initial board tile layout parsed from the TILES block.
  std::vector<TileState> loadedTiles;

  bool headerFound = false;
  bool endHeaderFound = false;
  bool inTilesBlock = false;

  // =========================================================
  // PASS 1: Read the HEADER block and reconstruct initialState
  //
  // This pass stops at END_HEADER.
  // It parses:
  // - board dimensions
  // - initial player positions
  // - side / phase / status / winner
  // - tile map from TILES ... END_TILES
  // =========================================================
  while (std::getline(file, line)) {
    if (line == "HEADER") {
      headerFound = true;
      continue;
    }

    if (line == "END_HEADER") {
      endHeaderFound = true;
      break;
    }

    if (!headerFound) {
      continue;
    }

    if (line == "TILES") {
      inTilesBlock = true;
      continue;
    }

    if (line == "END_TILES") {
      inTilesBlock = false;
      continue;
    }

    // If currently inside the tile block, parse tile values row by row.
    if (inTilesBlock) {
      std::istringstream iss(line);
      int tileValue = 0;

      while (iss >> tileValue) {
        if (tileValue == 0) {
          loadedTiles.push_back(TileState::Intact);
        } else if (tileValue == 1) {
          loadedTiles.push_back(TileState::Broken);
        } else {
          return std::nullopt;
        }
      }
      continue;
    }

    // Otherwise parse ordinary key=value header fields.
    try {
      if (line.rfind("rows=", 0) == 0) {
        rows = std::stoi(line.substr(5));
      } else if (line.rfind("cols=", 0) == 0) {
        cols = std::stoi(line.substr(5));
      } else if (line.rfind("p1_row=", 0) == 0) {
        p1Row = std::stoi(line.substr(7));
      } else if (line.rfind("p1_col=", 0) == 0) {
        p1Col = std::stoi(line.substr(7));
      } else if (line.rfind("p2_row=", 0) == 0) {
        p2Row = std::stoi(line.substr(7));
      } else if (line.rfind("p2_col=", 0) == 0) {
        p2Col = std::stoi(line.substr(7));
      } else if (line.rfind("side_to_move=", 0) == 0) {
        sideToMoveRaw = std::stoi(line.substr(13));
      } else if (line.rfind("phase=", 0) == 0) {
        phaseRaw = std::stoi(line.substr(6));
      } else if (line.rfind("status=", 0) == 0) {
        statusRaw = std::stoi(line.substr(7));
      } else if (line.rfind("winner=", 0) == 0) {
        winnerRaw = std::stoi(line.substr(7));
      } else if (line.rfind("player1_name=", 0) == 0) {
        player1Name = line.substr(13);
        if (!isValidPlayerName(player1Name)) {
          return std::nullopt;
        }
      } else if (line.rfind("player2_name=", 0) == 0) {
        player2Name = line.substr(13);
        if (!isValidPlayerName(player2Name)) {
          return std::nullopt;
        }
      }
    } catch (...) {
      return std::nullopt;
    }
  }

  // Validate that the header existed and was fully closed.
  if (!headerFound || !endHeaderFound) {
    return std::nullopt;
  }

  // Validate header values before constructing GameState.
  if (rows <= 0 || cols <= 0) {
    return std::nullopt;
  }

  if (p1Row < 0 || p1Row >= rows || p1Col < 0 || p1Col >= cols) {
    return std::nullopt;
  }

  if (p2Row < 0 || p2Row >= rows || p2Col < 0 || p2Col >= cols) {
    return std::nullopt;
  }

  if (sideToMoveRaw != 0 && sideToMoveRaw != 1) {
    return std::nullopt;
  }

  /*Only two players are supported in replay*/
  if (winnerRaw != -1 && winnerRaw != 0 && winnerRaw != 1) {
    return std::nullopt;
  }

  if (phaseRaw < 0 || phaseRaw > 2) {
    return std::nullopt;
  }

  if (statusRaw < 0 || statusRaw > 1) {
    return std::nullopt;
  }

  if (static_cast<int>(loadedTiles.size()) != rows * cols) {
    return std::nullopt;
  }

  // Build the initial GameState from parsed header data.
  GameState initialState(rows, cols);

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      Coord here{r, c};
      initialState.setTile(here, loadedTiles[r * cols + c]);
    }
  }

  initialState.setPlayerPos(Side::Player1, Coord{p1Row, p1Col});
  initialState.setPlayerPos(Side::Player2, Coord{p2Row, p2Col});
  initialState.setSideToMove(static_cast<Side>(sideToMoveRaw));
  initialState.setPhase(static_cast<TurnPhase>(phaseRaw));
  initialState.setStatus(static_cast<SessionStatus>(statusRaw));

  // =========================================================
  // PASS 2: Read the TURNS section
  //
  // At this point the stream is already positioned after END_HEADER,
  // so the remaining lines should be either:
  // - "TURNS"
  // - empty lines
  // - serialized turn records
  // =========================================================
  bool inTurns = false;
  bool inMessages = false;
  bool sawExplicitTurnSection = false;
  bool sawExplicitMessageSection = false;
  std::size_t expectedMessageCount = 0;
  bool messageCountSeen = false;

  while (std::getline(file, line)) {
    if (line.empty()) {
      continue;
    }
    if (line == "TURNS") {
      inTurns = true;
      inMessages = false;
      sawExplicitTurnSection = true;
      continue;
    }
    if (line == "END_TURNS") {
      inTurns = false;
      continue;
    }
    if (line == "UI_MESSAGES") {
      inMessages = true;
      inTurns = false;
      sawExplicitMessageSection = true;
      continue;
    }
    if (line == "END_UI_MESSAGES") {
      inMessages = false;
      continue;
    }

    if (inTurns) {
      std::istringstream iss(line);

      int actor = 0;
      int moveRow = 0;
      int moveCol = 0;
      int breakRow = 0;
      int breakCol = 0;
      int thinkMove = 0;
      int thinkBreak = 0;

      if (!(iss >> actor >> moveRow >> moveCol >> breakRow >> breakCol >>
            thinkMove >> thinkBreak)) {
        return std::nullopt;
      }

      if (actor != 0 && actor != 1) {
        return std::nullopt;
      }

      TurnRecord turn;
      turn.actor = static_cast<Side>(actor);
      turn.moveCoord = Coord{moveRow, moveCol};
      turn.breakCoord = Coord{breakRow, breakCol};
      turn.thinkTicksBeforeMove = thinkMove;
      turn.thinkTicksBeforeBreak = thinkBreak;

      history.push_back(turn);
      continue;
    }

    if (inMessages) {
      if (line.rfind("count=", 0) == 0) {
        try {
          expectedMessageCount =
              static_cast<std::size_t>(std::stoul(line.substr(6)));
        } catch (...) {
          return std::nullopt;
        }
        messageCountSeen = true;
        continue;
      }

      if (line.rfind("msg=", 0) == 0) {
        std::istringstream iss(line.substr(4));
        std::string msg;
        if (!(iss >> std::quoted(msg))) {
          return std::nullopt;
        }
        uiMessages.push_back(msg);
        continue;
      }

      return std::nullopt;
    }

    // Backward compatibility for old files:
    // after END_HEADER, old replays had raw turn lines directly.
    if (!sawExplicitTurnSection && !sawExplicitMessageSection) {
      std::istringstream iss(line);

      int actor = 0;
      int moveRow = 0;
      int moveCol = 0;
      int breakRow = 0;
      int breakCol = 0;
      int thinkMove = 0;
      int thinkBreak = 0;

      if (!(iss >> actor >> moveRow >> moveCol >> breakRow >> breakCol >>
            thinkMove >> thinkBreak)) {
        return std::nullopt;
      }

      if (actor != 0 && actor != 1) {
        return std::nullopt;
      }

      TurnRecord turn;
      turn.actor = static_cast<Side>(actor);
      turn.moveCoord = Coord{moveRow, moveCol};
      turn.breakCoord = Coord{breakRow, breakCol};
      turn.thinkTicksBeforeMove = thinkMove;
      turn.thinkTicksBeforeBreak = thinkBreak;

      history.push_back(turn);
      continue;
    }

    return std::nullopt;
  }

  if (messageCountSeen && uiMessages.size() != expectedMessageCount) {
    return std::nullopt;
  }

  // Return the fully reconstructed replay package.

  return ReplayData{initialState, history,     uiMessages,
                    winnerRaw,    player1Name, player2Name};
}
