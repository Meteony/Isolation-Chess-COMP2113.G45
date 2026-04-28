#include "sessions/match_session.hpp"

#include <array>
#include <random>
#include <string>

#include "core/game_rules.hpp"
#include "core/time.hpp"
#include "players/ai_player.hpp"
#include "players/human_player.hpp"

MatchSession MatchSession::CreateHumanVsHuman(
    int rows, int cols, std::string player1Name, std::string player2Name) {
  return MatchSession(rows, cols, new HumanPlayer(), new HumanPlayer(),
                      player1Name, player2Name);
}

MatchSession MatchSession::CreateHumanVsAi(int rows, int cols,
                                           AiDifficulty difficulty,
                                           std::string humanName,
                                           std::string aiName) {
  return MatchSession(rows, cols, new HumanPlayer(),
                      new AiPlayer(difficulty, Side::Player2), humanName,
                      aiName);
}

MatchSession MatchSession::CreateAiVsAi(
    int rows, int cols, AiDifficulty player1Difficulty,
    AiDifficulty player2Difficulty, std::string player1Name,
    std::string player2Name) {
  return MatchSession(rows, cols,
                      new AiPlayer(player1Difficulty, Side::Player1),
                      new AiPlayer(player2Difficulty, Side::Player2),
                      player1Name, player2Name);
}

MatchSession MatchSession::TakeOwnership(int rows, int cols, Player* p1,
                                         Player* p2,
                                         std::string player1Name,
                                         std::string player2Name) {
  return MatchSession(rows, cols, p1, p2, player1Name, player2Name);
}

/* Private initialization constructor. Use the public factories above. */
MatchSession::MatchSession(int rows, int cols, Player* p1, Player* p2,
                           std::string player1Name, std::string player2Name)
    : m_initialState(rows, cols),
      m_state(rows, cols),
      m_visualState{},
      m_p1(p1),
      m_p2(p2),
      m_player1Name(player1Name),
      m_player2Name(player2Name),
      m_gameTick{0},
      m_history{},
      m_currentTurnRecord{} {
  m_initialState.setSideToMove(Side::Player1);
  m_initialState.setPhase(TurnPhase::NewTurn);
  m_initialState.setStatus(SessionStatus::Running);

  m_state = m_initialState;
}

// Destructor
MatchSession::~MatchSession() {
  delete m_p1;
  delete m_p2;
}

const Player& MatchSession::currentPlayer() const {
  return (m_state.sideToMove() == Side::Player1) ? *m_p1 : *m_p2;
}

const std::string& MatchSession::playerName(Side side) const {
  return (side == Side::Player1) ? m_player1Name : m_player2Name;
}

static Side otherSide(Side side) {
  return (side == Side::Player1) ? Side::Player2 : Side::Player1;
}

static int legalMoveCount(const GameState& state, Side side) {
  int count = 0;
  const Coord pos = state.playerPos(side);
  for (int r = pos.row - 1; r <= pos.row + 1; ++r) {
    for (int c = pos.col - 1; c <= pos.col + 1; ++c) {
      if (GameRules::isLegalMove(state, side, {r, c})) {
        ++count;
      }
    }
  }
  return count;
}

static bool isAiPlayer(const Player& player) {
  return dynamic_cast<const AiPlayer*>(&player) != nullptr;
}

template <size_t N>
static std::string pickAiLine(int index,
                              const std::array<const char*, N>& lines) {
  return std::string(lines[static_cast<size_t>(index) % lines.size()]);
}

static bool aiChatRoll(int oneIn) {
  static std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> dist(1, oneIn);
  return dist(rng) == 1;
}

static std::string aiTurnStartLine(const GameState& state, Side aiSide,
                                   int gameTick) {
  const Side opponent = otherSide(aiSide);
  const int oppMoves = legalMoveCount(state, opponent);
  const int selfMoves = legalMoveCount(state, aiSide);

  if (oppMoves == 0) {
    return "Just one more square.";
  }
  if (oppMoves <= 1 && selfMoves >= 2) {
    return "You're almost out of air.";
  }
  if (selfMoves <= 1 && oppMoves >= 3) {
    return "Okay, this got awkward.";
  }

  static const std::array<const char*, 8> kLines = {
      "Let's see how you wriggle out of this.",
      "Try not to donate a free edge.",
      "I have seen sturdier plans in wet cardboard.",
      "Deep breath. Bad position.",
      "You can still pretend this is theory.",
      "Time to make the board unpleasant.",
      "Keep going, I'm collecting free squares.",
      "This line already smells winning.",
  };
  return pickAiLine((gameTick / 7) + static_cast<int>(aiSide), kLines);
}

static std::string aiMoveLine(const GameState& state, Side aiSide,
                              int gameTick) {
  const Side opponent = otherSide(aiSide);
  const int oppMoves = legalMoveCount(state, opponent);
  const int selfMoves = legalMoveCount(state, aiSide);

  if (oppMoves <= 1) {
    return "That should narrow your options nicely.";
  }
  if (selfMoves <= 1) {
    return "I'm calling human resources";
  }

  static const std::array<const char*, 8> kLines = {
      "Misclick.",
      "Nice board. Mine now.",
      "You really left that open.",
      "That move seemed like too much work for you.",
      "I like this geometry better.",
      "Small step, large headache.",
      "Disaster averted.",
      "Guess what? C",
  };
  return pickAiLine((gameTick / 5) + static_cast<int>(aiSide), kLines);
}

static std::string aiBreakLine(const GameState& state, Side aiSide,
                               int gameTick) {
  const Side opponent = otherSide(aiSide);
  const int oppMoves = legalMoveCount(state, opponent);
  const int selfMoves = legalMoveCount(state, aiSide);

  if (oppMoves == 0) {
    return "Ez.";
  }
  if (oppMoves <= 1 && selfMoves >= 2) {
    return "What is he cooking?";
  }
  if (selfMoves <= 1 && oppMoves >= 3) {
    return "WAIWAIWAIWAIAWI CHILL UOT";
  }

  static const std::array<const char*, 11> kLines = {
      "Who carried you to this level? ",
      "My grandma plays better than you tbh and I like her. ",
      "readable pattern",
      "I could win this with my eyes closed",
      "I have seen sturdier plans in spaghetti.",
      "you walked into that",
      "too slow on the switch",
      "bold move, wrong board",
      "Beep. Vitality check.",
      "absolute clown move",
      "Kinda regarded tbh."};
  return pickAiLine((gameTick / 9) + static_cast<int>(aiSide), kLines);
}

static std::string aiWinnerLine(int gameTick) {
  static const std::array<const char*, 6> kLines = {
      "gg",      "nice try",    "Who carried you to this level?",
      "mid tbh", "Checks out.", "\"Does he know?\"",
  };
  return pickAiLine(gameTick / 11, kLines);
}

static std::string aiLoserLine(int gameTick) {
  static const std::array<const char*, 6> kLines = {
      "I'm calling human resources.",
      "I hope you stub your toes.",
      "ggwp.",
      "I had that coming.",
      "I'm in ur walls.",
      "rigged??????????",
  };
  return pickAiLine(gameTick / 11, kLines);
}

void MatchSession::update(int inputChar) {
  ++m_gameTick;

  Player& player{currentPlayer()};

  auto formatTicks = [](long ticks) {
    const long whole = ticks / kGameFps;
    const long hundredths = (ticks % kGameFps) * 100 / kGameFps;

    if (hundredths == 0) {
      return std::to_string(whole) + "s";
    }

    std::string frac = std::to_string(hundredths);
    if (hundredths < 10) frac = "0" + frac;
    return std::to_string(whole) + "." + frac + "s";
  };

  auto coloredPlayerName = [&](Side side) {
    return ((side == Side::Player1) ? std::string("<P1>")
                                    : std::string("<P2>")) +
           playerName(side);
  };

  // If the bot is going to be smug, it can at least use the same chat lane
  // as everybody else instead of graffitiing raw color tags into the log.
  // Also, keep the yap budget low. This clown is not paid by the line.
  auto postSpokenLine = [&](Side speaker, const std::string& text) {
    if (text.empty()) {
      return;
    }
    postUiMessage(coloredPlayerName(speaker) + ": " + text);
  };

  if (m_state.status() == SessionStatus::Finished) {
    m_visualState.cursorVisible = false;
    goto UpdateAndReturn;
  }

  switch (m_state.phase()) {
    case TurnPhase::NewTurn: {
      m_currentTurnRecord.reset(m_state.sideToMove());

      // negative-start trick: later do += m_gameTick
      m_currentTurnRecord.thinkTicksBeforeMove = -m_gameTick;

      m_state.setPhase(TurnPhase::Move);
      player.beginMovePhase(m_state);

      int currentTurnNumber = static_cast<int>(m_history.size()) + 1;
      std::string msg =
          "<YELLOW>[i] Turn <YELLOW>" + std::to_string(currentTurnNumber);
      postUiMessage(msg);

      if (isAiPlayer(player)) {
        const Side aiSide = m_state.sideToMove();
        if (aiChatRoll(4)) {
          postSpokenLine(aiSide, aiTurnStartLine(m_state, aiSide, m_gameTick));
        }
      }

      goto UpdateAndReturn;
    }

    case TurnPhase::Move: {
      player.update(inputChar, m_state);

      if (!player.hasMoveReady()) {
        goto UpdateAndReturn;
      }

      Coord move = player.consumeMove();

      if (!GameRules::isLegalMove(m_state, m_state.sideToMove(), move)) {
        goto UpdateAndReturn;
      }

      GameRules::applyMove(m_state, m_state.sideToMove(), move);

      m_currentTurnRecord.moveCoord = move;
      m_currentTurnRecord.thinkTicksBeforeMove += m_gameTick;

      if (isAiPlayer(player)) {
        const Side aiSide = m_state.sideToMove();
        if (aiChatRoll(14)) {
          postSpokenLine(aiSide, aiMoveLine(m_state, aiSide, m_gameTick));
        }
      }

      // begin timing break phase now
      m_currentTurnRecord.thinkTicksBeforeBreak = -m_gameTick;

      player.beginBreakPhase(m_state);
      m_state.setPhase(TurnPhase::Break);
      goto UpdateAndReturn;
    }

    case TurnPhase::Break: {
      player.update(inputChar, m_state);

      if (!player.hasBreakReady()) {
        goto UpdateAndReturn;
      }

      Coord breakTile = player.consumeBreak();

      if (!GameRules::isLegalBreak(m_state, breakTile)) {
        goto UpdateAndReturn;
      }

      GameRules::applyBreak(m_state, breakTile);

      m_currentTurnRecord.breakCoord = breakTile;
      m_currentTurnRecord.thinkTicksBeforeBreak += m_gameTick;

      const Side actor = m_state.sideToMove();
      const long moveTicks = m_currentTurnRecord.thinkTicksBeforeMove;
      const long breakTicks = m_currentTurnRecord.thinkTicksBeforeBreak;
      const long totalTicks = moveTicks + breakTicks;
      postUiMessage(coloredPlayerName(actor) + ": Finished in " +
                    formatTicks(totalTicks));

      if (isAiPlayer(player) && aiChatRoll(3)) {
        postSpokenLine(actor, aiBreakLine(m_state, actor, m_gameTick));
      }

      pushHistory(m_currentTurnRecord);

      Side nextSide = otherSide(m_state.sideToMove());

      const Side winner = !GameRules::hasAnyLegalMove(m_state, nextSide)
                              ? m_state.sideToMove()
                              : nextSide;

      if (!GameRules::hasAnyLegalMove(m_state, nextSide) ||
          !GameRules::hasAnyLegalMove(m_state, m_state.sideToMove())) {
        m_state.setWinner(winner);
        m_state.setStatus(SessionStatus::Finished);

        if (isAiPlayer(player) && winner == actor) {
          postSpokenLine(actor, aiWinnerLine(m_gameTick));
        } else {
          Player& loser = (winner == Side::Player1) ? *m_p2 : *m_p1;
          if (isAiPlayer(loser)) {
            postSpokenLine(otherSide(winner), aiLoserLine(m_gameTick));
          }
        }

        postUiMessage(std::string("<YELLOW>Result: ") +
                      coloredPlayerName(winner) + std::string(" wins."));
        goto UpdateAndReturn;
      }

      m_state.setSideToMove(nextSide);
      m_state.setPhase(TurnPhase::NewTurn);
      goto UpdateAndReturn;
    }
  }

UpdateAndReturn:
  // Keep main's shared exit funnel intact. Yes, the goto stays.
  if (auto* human = dynamic_cast<HumanPlayer*>(&player)) {
    m_visualState.cursorVisible = true;
    m_visualState.cursor = human->cursor();
  } else {
    m_visualState.cursorVisible = false;
  }
  return;
}

// Return current player ptr
Player& MatchSession::currentPlayer() {
  return (m_state.sideToMove() == Side::Player1) ? *m_p1 : *m_p2;
}

void MatchSession::pushHistory(const TurnRecord& record) {
  m_history.push_back(record);
}

const MatchVisualState& MatchSession::visualState() const {
  return m_visualState;
};

ReplayData MatchSession::buildReplayData() const {
  const int winner = (m_state.status() == SessionStatus::Finished)
                         ? static_cast<int>(m_state.winner())
                         : -1;

  return ReplayData{m_initialState, m_history,     m_uiMessages,
                    winner,         m_player1Name, m_player2Name};
}

const GameState& MatchSession::state() const { return m_state; }

TurnPhase MatchSession::phase() const { return m_state.phase(); }

bool MatchSession::isFinished() const {
  return m_state.status() == SessionStatus::Finished;
}
