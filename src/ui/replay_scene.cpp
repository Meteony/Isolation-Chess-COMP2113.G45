#include "ui/replay_scene.hpp"
#include "app/app.hpp"

#include "sessions/replay_session.hpp" // <-- IMPORTANT: complete type here

#include <ncurses.h>
#include <utility>

ReplayScene::ReplayScene(std::unique_ptr<ReplaySession> session)
    : m_session(std::move(session)) {}

ReplayScene::~ReplayScene() = default;

void ReplayScene::handleInput(App &app, int ch)
{
    if (ch == 'q' || ch == 'Q')
    {
        app.popToMainMenu();
    }
}

void ReplayScene::update(App &app)
{
    (void)app;
}

void ReplayScene::render(App &app)
{
    werase(app.boardWin());
    werase(app.hudWin());
    box(app.boardWin(), 0, 0);
    box(app.hudWin(), 0, 0);
    mvwprintw(app.boardWin(), 1, 2, "ReplayScene not implemented yet.");
    wrefresh(app.boardWin());
    wrefresh(app.hudWin());
}