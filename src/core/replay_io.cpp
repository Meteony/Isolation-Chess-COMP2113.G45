#include "core/replay_io.hpp"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

const std::string ReplayIO::REPLAY_DIR = "replays";

bool ReplayIO::ensureDirectoryExists()
{
    try
    {
        if (!std::filesystem::exists(REPLAY_DIR))
        {
            std::filesystem::create_directory(REPLAY_DIR);
        }
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

std::string ReplayIO::generateFilename()
{
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char buffer[64];
    strftime(buffer, sizeof(buffer), "replay_%Y%m%d_%H%M%S.txt", &timeinfo);

    return std::string(REPLAY_DIR) + "/" + std::string(buffer);
}

bool ReplayIO::saveReplay(int rows,
                          int cols,
                          const GameState& initialState,
                          const std::vector<TurnRecord>& history,
                          std::string& outFilename)
{
    if (!ensureDirectoryExists())
    {
        return false;
    }

    outFilename = generateFilename();

    std::ofstream file(outFilename);
    if (!file.is_open())
    {
        return false;
    }

    file << "HEADER\n";
    file << "rows=" << rows << "\n";
    file << "cols=" << cols << "\n";

    Coord p1Pos = initialState.playerPos(Side::Player1);
    file << "p1_row=" << p1Pos.row << "\n";
    file << "p1_col=" << p1Pos.col << "\n";

    Coord p2Pos = initialState.playerPos(Side::Player2);
    file << "p2_row=" << p2Pos.row << "\n";
    file << "p2_col=" << p2Pos.col << "\n";

    file << "END_HEADER\n";

    file << "TURNS\n";
    for (const auto& turn : history)
    {
        int actor;
        if (turn.actor == Side::Player1)
        {
            actor = 0;
        }
        else
        {
            actor = 1;
        }

        file << actor << " "
             << turn.moveCoord.row << " " << turn.moveCoord.col << " "
             << turn.breakCoord.row << " " << turn.breakCoord.col << " "
             << turn.thinkTicksBeforeMove << " " << turn.thinkTicksBeforeBreak << "\n";
    }

    file.close();
    return true;
}

bool ReplayIO::loadReplay(const std::string& filepath,
                          ReplayData& outData)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        return false;
    }

    outData.history.clear();

    std::string line;
    int p1Row = 0;
    int p1Col = 0;
    int p2Row = 0;
    int p2Col = 0;

    bool headerFound = false;
    bool endHeaderFound = false;

    while (std::getline(file, line))
    {
        if (line == "HEADER")
        {
            headerFound = true;
            continue;
        }

        if (line == "END_HEADER")
        {
            endHeaderFound = true;
            break;
        }

        if (!headerFound)
        {
            continue;
        }

        try
        {
            if (line.find("rows=") == 0)
            {
                outData.rows = std::stoi(line.substr(5));
            }
            else if (line.find("cols=") == 0)
            {
                outData.cols = std::stoi(line.substr(5));
            }
            else if (line.find("p1_row=") == 0)
            {
                p1Row = std::stoi(line.substr(7));
            }
            else if (line.find("p1_col=") == 0)
            {
                p1Col = std::stoi(line.substr(7));
            }
            else if (line.find("p2_row=") == 0)
            {
                p2Row = std::stoi(line.substr(7));
            }
            else if (line.find("p2_col=") == 0)
            {
                p2Col = std::stoi(line.substr(7));
            }
        }
        catch (const std::exception&)
        {
            file.close();
            return false;
        }
    }

    if (!headerFound || !endHeaderFound)
    {
        file.close();
        return false;
    }

    if (outData.rows <= 0 || outData.cols <= 0)
    {
        file.close();
        return false;
    }

    if (p1Row < 0 || p1Row >= outData.rows || p1Col < 0 || p1Col >= outData.cols ||
        p2Row < 0 || p2Row >= outData.rows || p2Col < 0 || p2Col >= outData.cols)
    {
        file.close();
        return false;
    }

    outData.initialState = GameState(outData.rows, outData.cols);
    outData.initialState.setPlayerPos(Side::Player1, Coord{p1Row, p1Col});
    outData.initialState.setPlayerPos(Side::Player2, Coord{p2Row, p2Col});
    outData.initialState.setSideToMove(Side::Player1);
    outData.initialState.setPhase(TurnPhase::NewTurn);
    outData.initialState.setStatus(SessionStatus::Running);

    while (std::getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        if (line == "TURNS")
        {
            continue;
        }

        std::istringstream iss(line);
        int actor;
        int moveRow;
        int moveCol;
        int breakRow;
        int breakCol;
        long thinkMove;
        long thinkBreak;

        if (!(iss >> actor >> moveRow >> moveCol >> breakRow >> breakCol >> thinkMove >> thinkBreak))
        {
            file.close();
            return false;
        }

        if (actor != 0 && actor != 1)
        {
            file.close();
            return false;
        }

        TurnRecord turn;

        if (actor == 0)
        {
            turn.actor = Side::Player1;
        }
        else
        {
            turn.actor = Side::Player2;
        }

        turn.moveCoord.row = moveRow;
        turn.moveCoord.col = moveCol;
        turn.breakCoord.row = breakRow;
        turn.breakCoord.col = breakCol;
        turn.thinkTicksBeforeMove = thinkMove;
        turn.thinkTicksBeforeBreak = thinkBreak;

        outData.history.push_back(turn);
    }

    file.close();
    return true;
}
