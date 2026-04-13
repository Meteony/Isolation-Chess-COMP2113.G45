#include "ui/live_game_scene.hpp"
#include "app/app.hpp"

#include "sessions/match_session.hpp"
#include "players/human_player.hpp"
#include "players/ai_player.hpp"

#include <ncurses.h>
#include <memory>

std::unique_ptr<LiveGameScene> LiveGameScene::makeHumanVsHuman(int rows, int cols)
{
    auto scene = std::unique_ptr<LiveGameScene>(new LiveGameScene());

    scene->m_p1 = std::make_unique<HumanPlayer>();
    scene->m_p2 = std::make_unique<HumanPlayer>();

    scene->m_session = std::make_unique<MatchSession>(
        rows, cols, scene->m_p1.get(), scene->m_p2.get());

    return scene;
}

std::unique_ptr<LiveGameScene> LiveGameScene::makeHumanVsAi(int rows, int cols, AiDifficulty difficulty)
{
    auto scene = std::unique_ptr<LiveGameScene>(new LiveGameScene());

    scene->m_p1 = std::make_unique<HumanPlayer>();
    scene->m_p2 = std::make_unique<AiPlayer>(difficulty, Side::Player2);

    scene->m_session = std::make_unique<MatchSession>(
        rows, cols, scene->m_p1.get(), scene->m_p2.get());

    return scene;
}

void LiveGameScene::handleInput(App &app, int ch)
{
    if (ch == 'q' || ch == 'Q')
    {
        app.popToMainMenu();
        return;
    }

    // Always tick the session; HumanPlayer ignores ERR internally.
    m_session->update(ch);
}

void LiveGameScene::update(App &app)
{
    (void)app;

    const MatchVisualState &vs = m_session->visualState();
    m_overlay.cursorVisible = vs.cursorVisible;
    m_overlay.cursor = vs.cursor;
    m_overlay.phaseHint = m_session->phase();
}

void LiveGameScene::render(App &app)
{
    m_renderer.draw(app.boardWin(), app.hudWin(), *m_session, m_overlay, m_vfx);
}