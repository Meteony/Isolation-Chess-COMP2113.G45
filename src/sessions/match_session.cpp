#include "sessions/match_session.hpp"
#include "players/human_player.hpp"
#include "core/game_rules.hpp"

// Initialization Constructor
MatchSession::MatchSession(int rows, int cols, Player* p1, Player* p2)
    : m_state(rows, cols),
      m_visualState{},
      m_p1(p1),
      m_p2(p2),
      m_gameTick{0},
      m_history{},
      m_currentTurnRecord{}
{
    m_state.setSideToMove(Side::Player1);
    m_state.setPhase(TurnPhase::NewTurn);
    m_state.setStatus(SessionStatus::Running);
}

// Destructor
MatchSession::~MatchSession() {
    delete m_p1;
    delete m_p2;
}

const Player& MatchSession::currentPlayer() const {
    return (m_state.sideToMove() == Side::Player1) ? *m_p1 : *m_p2;
}

static Side otherSide(Side side) {
    return (side == Side::Player1) ? Side::Player2 : Side::Player1;
}

void MatchSession::update(int inputChar){
    ++m_gameTick;

    Player& player{currentPlayer()};

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
            return;
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

            pushHistory(m_currentTurnRecord);

            Side nextSide = otherSide(m_state.sideToMove());

            if (!GameRules::hasAnyLegalMove(m_state, nextSide)) {
                m_state.setWinner(m_state.sideToMove());
                m_state.setStatus(SessionStatus::Finished);
                return;
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


const std::vector<TurnRecord>& MatchSession::history() const {
    return m_history;
}

const GameState& MatchSession::state() const {
    return m_state;
}

TurnPhase MatchSession::phase() const {
    return m_state.phase();
}

bool MatchSession::isFinished() const {
    return m_state.status() == SessionStatus::Finished;
}




