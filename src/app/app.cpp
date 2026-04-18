#include "app/app.hpp"

#include "ui/scene.hpp"
#include "ui/main_menu_scene.hpp"
#include "ui/live_game_scene.hpp"

#include "core/enums.hpp"
#include <utility> // std::move

App::App() = default;

App::~App()
{
    destroyWindows();
    shutdownCurses();
}

int App::run()
{
    initCurses();
    createWindows();

    setScene(std::make_unique<MainMenuScene>());

    while (m_running)
    {
        updateTerminalSize();

        int ch = getch(); // ERR when nodelay() and no input

        if (m_scene)
        {
            m_scene->handleInput(*this, ch);
            m_scene->update(*this);
            m_scene->render(*this);
        }

        napms(16);
    }

    return 0;
}

void App::setScene(std::unique_ptr<Scene> scene)
{
    if (m_scene)
        m_scene->onExit(*this);
    m_scene = std::move(scene);
    if (m_scene)
        m_scene->onEnter(*this);
}

void App::requestQuit()
{
    m_running = false;
}

void App::popToMainMenu()
{
    setScene(std::make_unique<MainMenuScene>());
}

void App::startNewHumanGame()
{
    // Adjust board size if your game is not 8x8
    setScene(LiveGameScene::makeHumanVsHuman(8, 8));
}

void App::startNewAiGame(AiDifficulty difficulty)
{
    setScene(LiveGameScene::makeHumanVsAi(8, 8, difficulty));
}

void App::initCurses()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    // smooth UI loop
    nodelay(stdscr, TRUE);
}

void App::shutdownCurses()
{
    if (stdscr)
        endwin();
}

void App::updateTerminalSize()
{
    getmaxyx(stdscr, m_termH, m_termW);
}

void App::createWindows()
{
    updateTerminalSize();

    int boardW = m_termW - m_hudW;
    if (boardW < 20)
        boardW = m_termW; // fallback

    destroyWindows();

    m_boardWin = newwin(m_termH, boardW, 0, 0);

    if (boardW == m_termW)
    {
        // no space for HUD; make a tiny window at right edge
        m_hudWin = newwin(m_termH, 1, 0, boardW - 1);
    }
    else
    {
        m_hudWin = newwin(m_termH, m_hudW, 0, boardW);
    }

    keypad(m_boardWin, TRUE);
    keypad(m_hudWin, TRUE);
}

void App::destroyWindows()
{
    if (m_boardWin)
    {
        delwin(m_boardWin);
        m_boardWin = nullptr;
    }
    if (m_hudWin)
    {
        delwin(m_hudWin);
        m_hudWin = nullptr;
    }
}