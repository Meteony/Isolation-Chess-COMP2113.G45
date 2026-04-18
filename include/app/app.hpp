#pragma once

#include <memory>
#include <ncurses.h>

class Scene;
enum class AiDifficulty;

class App
{
public:
    App();
    ~App();

    App(const App &) = delete;
    App &operator=(const App &) = delete;

    // Initialize ncurses, create windows, run scene loop until quit.
    int run();

    // Scene control
    void setScene(std::unique_ptr<Scene> scene);
    void requestQuit();
    void popToMainMenu();

    // Game start helpers called from MainMenuScene
    void startNewHumanGame();
    void startNewAiGame(AiDifficulty difficulty);

    // Windows for renderers/scenes
    WINDOW *boardWin() const { return m_boardWin; }
    WINDOW *hudWin() const { return m_hudWin; }

    // Terminal size (best-effort)
    int termH() const { return m_termH; }
    int termW() const { return m_termW; }

private:
    void initCurses();
    void shutdownCurses();

    void createWindows();
    void destroyWindows();
    void updateTerminalSize();

private:
    bool m_running = true;
    std::unique_ptr<Scene> m_scene;

    WINDOW *m_boardWin = nullptr;
    WINDOW *m_hudWin = nullptr;

    int m_termH = 0;
    int m_termW = 0;

    // HUD width (right side)
    int m_hudW = 28;
};