#include "sessions/match_session.hpp"

#include <sstream>

#include "core/game_rules.hpp"
#include "core/time.hpp"
#include "players/ai_player.hpp"
#include "players/human_player.hpp"

/* Initialization Constructor. Names optional */
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

static std::string colorizeAiSpeech(const std::string& text) {
  std::istringstream iss(text);
  std::ostringstream oss;
  std::string word;
  bool first = true;

  while (iss >> word) {
    if (!first) {
      oss << ' ';
    }
    oss << "<MAGENTA>" << word;
    first = false;
  }

  return first ? std::string{"<MAGENTA>AI:"} : oss.str();
}

static std::string aiReactionForPosition(const GameState& state, Side aiSide,
                                         int gameTick) {
  const Side opponent = otherSide(aiSide);
  const int oppMoves = legalMoveCount(state, opponent);
  const int selfMoves = legalMoveCount(state, aiSide);

  if (oppMoves == 0) {
    return "AI: ez";
  }

  if (oppMoves <= 1 && selfMoves >= 2) {
    return "AI: one move left";
  }

  if (selfMoves <= 1 && oppMoves >= 3) {
    return "AI: wait...";
  }

  switch ((gameTick / 9) % 22) {
    case 0:
      return "AI: your turn";
    case 1:
      return "AI: still easy";
    case 2:
      return "AI: Who carried you to this level?";
    case 3:
      return "AI: My grandma plays better than you in wheelchair ngl";
    case 4:
      return "AI: nice try";
    case 5:
      return "AI: mid tbh";
    case 6:
      return "AI: pressure on";
    case 7:
      return "AI: Nice try, I guess...";
    case 8:
      return "AI: good defense";
    case 9:
      return "AI: that's interesting";
    case 10:
      return "AI: readable pattern";
    case 11:
      return "AI: I could win this with my eyes closed";
    case 12:
      return "AI: edge secured";
    case 13:
      return "AI: keep it coming";
    case 14:
      return "AI: calculation check";
    case 15:
      return "AI: that was a free square";
    case 16:
      return "AI: bold move, wrong board";
    case 17:
      return "AI: you walked into that";
    case 18:
      return "AI: too slow on the switch";
    case 19:
      return "AI: that's a rough line";
    case 20:
      return "AI: I saw that coming";
    case 21:
      return "AI: Kinda retarded tbh";
    default:
      return "AI: absolute clown move";
  }
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

  auto coloredPlayerName = [&](Side side) { /*For auto UI turn messages too*/
                                            return ((side == Side::Player1)
                                                        ? std::string("<P1>")
                                                        : std::string("<P2>")) +
                                                   playerName(side);
  };

  // Update UI element
  if (auto* human = dynamic_cast<HumanPlayer*>(&player)) {
    m_visualState.cursorVisible = true;
    m_visualState.cursor = human->cursor();
  } else {
    m_visualState.cursorVisible = false;
  }

  if (m_state.status() == SessionStatus::Finished) {
    m_visualState.cursorVisible = false;
    return;
  }

  switch (m_state.phase()) {
    case TurnPhase::NewTurn: {
      m_currentTurnRecord.reset(m_state.sideToMove());

      // negative-start trick: later do += m_gameTick
      m_currentTurnRecord.thinkTicksBeforeMove = -m_gameTick;

      m_state.setPhase(TurnPhase::Move);
      player.beginMovePhase(m_state);

      int currentTurnNumber = /*Wait we don't do turn tracking?*/
          static_cast<int>(m_history.size()) + 1;
      std::string msg =
          "<YELLOW>[i] Turn <YELLOW>" + std::to_string(currentTurnNumber);
      postUiMessage(msg);

      goto UpdateAndReturn;
    }

    case TurnPhase::Move: {
      player.update(inputChar, m_state);

      if (!player.hasMoveReady()) {
        return;
      }

      Coord move = player.consumeMove();

      if (!GameRules::isLegalMove(m_state, m_state.sideToMove(), move)) {
        return;
      }

      GameRules::applyMove(m_state, m_state.sideToMove(), move);

      m_currentTurnRecord.moveCoord = move;
      m_currentTurnRecord.thinkTicksBeforeMove += m_gameTick;

      if (isAiPlayer(player)) {
        const Side aiSide = m_state.sideToMove();
        const int selfMoves = legalMoveCount(m_state, aiSide);
        if (selfMoves <= 2) {
          postUiMessage(colorizeAiSpeech("AI: locked in"));
        }
      }

      // begin timing break phase now
      m_currentTurnRecord.thinkTicksBeforeBreak = -m_gameTick;

      player.beginBreakPhase(m_state);
      m_state.setPhase(TurnPhase::Break);
      return;
    }

    case TurnPhase::Break: {
      player.update(inputChar, m_state);

      if (!player.hasBreakReady()) {
        return;
      }

      Coord breakTile = player.consumeBreak();

      if (!GameRules::isLegalBreak(m_state, breakTile)) {
        return;
      }

      GameRules::applyBreak(m_state, breakTile);

      m_currentTurnRecord.breakCoord = breakTile;
      m_currentTurnRecord.thinkTicksBeforeBreak += m_gameTick;

      if (isAiPlayer(player)) {
        postUiMessage(colorizeAiSpeech(
            aiReactionForPosition(m_state, m_state.sideToMove(), m_gameTick)));
      }

      /*Post messages*/
      const Side actor = m_state.sideToMove();
      const long moveTicks = m_currentTurnRecord.thinkTicksBeforeMove;
      const long breakTicks = m_currentTurnRecord.thinkTicksBeforeBreak;
      const long totalTicks = moveTicks + breakTicks;
      postUiMessage(coloredPlayerName(actor) + ": Finished in " +
                    formatTicks(totalTicks));
      pushHistory(m_currentTurnRecord);

      Side nextSide = otherSide(m_state.sideToMove());

      if (!GameRules::hasAnyLegalMove(m_state, nextSide)) {
        m_state.setWinner(m_state.sideToMove());
        m_state.setStatus(SessionStatus::Finished);

        if (isAiPlayer(player)) {
          postUiMessage(colorizeAiSpeech("AI: ez"));
        } else {
          Player& loser = (nextSide == Side::Player1) ? *m_p1 : *m_p2;
          if (isAiPlayer(loser)) {
            postUiMessage(colorizeAiSpeech("AI: gg"));
          }
        }
        postUiMessage(std::string("<YELLOW>Result: ") +
                      coloredPlayerName(winner) + std::string(" wins."));
        goto UpdateAndReturn;
      }

      m_state.setSideToMove(nextSide);
      m_state.setPhase(TurnPhase::NewTurn);
      return;
    }
  }
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
