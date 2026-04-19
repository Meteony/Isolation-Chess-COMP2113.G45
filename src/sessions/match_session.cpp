#include "sessions/match_session.hpp"

#include <string>

#include "core/game_rules.hpp"
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

void MatchSession::update(int inputChar) {
  ++m_gameTick;

  Player& player{currentPlayer()};

  auto formatTicks = [](long ticks) { /*For auto UI turn messages*/
                                      if (ticks % 10 == 0) {
                                        return std::to_string(ticks / 10) + "s";
                                      }
                                      return std::to_string(ticks / 10) + "." +
                                             std::to_string(ticks % 10) + "s";
  };

  auto coloredPlayerName =
      [&](Side side) { /*For auto UI turn messages too*/
                       return ((side == Side::Player1)
                                   ? std::string("<YELLOW>")
                                   : std::string("<BLUE>")) +
                              playerName(side);
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

      int currentTurnNumber = /*Wait we don't do turn tracking?*/
          static_cast<int>(m_history.size()) + 1;
      std::string msg =
          "<MAGENTA>[i] Turn <MAGENTA>" + std::to_string(currentTurnNumber);
      postUiMessage(msg);

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

      /*Post messages*/ /*Well feels a bit too much I'm removing this message*/
      /*
      const long moveTicks = m_currentTurnRecord.thinkTicksBeforeMove;
      const std::string moveSeconds =
          (moveTicks % 10 == 0) ? std::to_string(moveTicks / 10) + "s"
                                : std::to_string(moveTicks / 10) + "." +
                                      std::to_string(moveTicks % 10) + "s";
      postUiMessage(((m_state.sideToMove() == Side::Player1) ? "<BLUE>P1: "
                                                             : "<RED>P2: ") +
                    std::string("Moved to <YELLOW>(") +
                    std::to_string(move.row) + ", <YELLOW>" +
                    std::to_string(move.col) + ") in <YELLOW>" + moveSeconds +
                    ".");
      */
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

      /*Post messages*/
      const Side actor = m_state.sideToMove();
      const long moveTicks = m_currentTurnRecord.thinkTicksBeforeMove;
      const long breakTicks = m_currentTurnRecord.thinkTicksBeforeBreak;
      const long totalTicks = moveTicks + breakTicks;
      postUiMessage(
          coloredPlayerName(actor) + std::string(": Moved to <YELLOW>(") +
          std::to_string(m_currentTurnRecord.moveCoord.row) + ", <YELLOW>" +
          std::to_string(m_currentTurnRecord.moveCoord.col) +
          ") and broke <YELLOW>(" + std::to_string(breakTile.row) +
          ", <YELLOW>" + std::to_string(breakTile.col) + ") in <YELLOW>" +
          formatTicks(totalTicks) + ".");

      pushHistory(m_currentTurnRecord);

      Side nextSide = otherSide(m_state.sideToMove());

      const Side winner = !GameRules::hasAnyLegalMove(m_state, nextSide)
                              ? m_state.sideToMove()
                              : nextSide;

      if (!GameRules::hasAnyLegalMove(m_state, nextSide) ||
          !GameRules::hasAnyLegalMove(m_state, m_state.sideToMove())) {
        m_state.setWinner(winner);
        m_state.setStatus(SessionStatus::Finished);
        postUiMessage(std::string("<MAGENTA>Result: ") +
                      coloredPlayerName(winner) + std::string(" wins."));
        goto UpdateAndReturn;
      }

      m_state.setSideToMove(nextSide);
      m_state.setPhase(TurnPhase::NewTurn);
      goto UpdateAndReturn;
    }
  }

UpdateAndReturn:
  // Update UI element
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
