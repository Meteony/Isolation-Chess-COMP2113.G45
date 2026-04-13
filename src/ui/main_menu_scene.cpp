/*
#include "ui/main_menu_scene.hpp"
#include <ncurses.h>
#include <string>
#include <vector>

// just set everything to defaults
MainMenuScene::MainMenuScene()
    : m_selectedOption(0),
      m_inDifficultyMenu(false),
      m_selectedDifficulty(0)
{
}

// keeps looping until the player confirms a choice
MenuChoice MainMenuScene::run() {
    // make getch() blocking so we just wait for input (no busy loop needed)
    nodelay(stdscr, false);
    keypad(stdscr, TRUE);
    curs_set(0);  // hide the blinking cursor

    while (true) {
        erase();

        // draw whichever menu screen we're on
        if (m_inDifficultyMenu) {
            drawDifficultyMenu();
        } else {
            drawMainMenu();
        }

        refresh();

        int ch = getch();

        // ---- MAIN MENU INPUT ----
        if (!m_inDifficultyMenu) {
            switch (ch) {
                case KEY_UP:
                case 'w':
                case 'W':
                    // move highlight up, but don't go past the first option
                    if (m_selectedOption > 0)
                        m_selectedOption--;
                    break;

                case KEY_DOWN:
                case 's':
                case 'S':
                    // move highlight down, cap at last option (index 2)
                    if (m_selectedOption < 2)
                        m_selectedOption++;
                    break;

                case '\n':
                case '\r':
                case KEY_ENTER:
                case 'c':
                case 'C': {
                    // user confirmed their pick
                    if (m_selectedOption == 0) {
                        // human vs human
                        MenuChoice choice;
                        choice.mode = MenuChoice::HumanVsHuman;
                        return choice;
                    }
                    else if (m_selectedOption == 1) {
                        // go to difficulty submenu
                        m_inDifficultyMenu = true;
                        m_selectedDifficulty = 0;
                    }
                    else if (m_selectedOption == 2) {
                        // quit
                        MenuChoice choice;
                        choice.mode = MenuChoice::Quit;
                        return choice;
                    }
                    break;
                }

                case 'q':
                case 'Q': {
                    // quick quit shortcut
                    MenuChoice choice;
                    choice.mode = MenuChoice::Quit;
                    return choice;
                }
            }
        }
        // ---- DIFFICULTY SUBMENU INPUT ----
        else {
            switch (ch) {
                case KEY_UP:
                case 'w':
                case 'W':
                    if (m_selectedDifficulty > 0)
                        m_selectedDifficulty--;
                    break;

                case KEY_DOWN:
                case 's':
                case 'S':
                    // 4 options total (Easy, Medium, Hard, Back) so cap at index 3
                    if (m_selectedDifficulty < 3)
                        m_selectedDifficulty++;
                    break;

                case '\n':
                case '\r':
                case KEY_ENTER:
                case 'c':
                case 'C': {
                    if (m_selectedDifficulty == 3) {
                        // "Back" — go back to main menu
                        m_inDifficultyMenu = false;
                    }
                    else {
                        // player picked a difficulty, build the choice and return it
                        MenuChoice choice;
                        choice.mode = MenuChoice::HumanVsAi;

                        if (m_selectedDifficulty == 0)
                            choice.difficulty = AiDifficulty::Easy;
                        else if (m_selectedDifficulty == 1)
                            choice.difficulty = AiDifficulty::Medium;
                        else
                            choice.difficulty = AiDifficulty::Hard;

                        return choice;
                    }
                    break;
                }

                case 27:  // ESC
                case 'x':
                case 'X':
                    // go back to main menu
                    m_inDifficultyMenu = false;
                    break;
            }
        }
    }
}

// draws the main menu with title and the three options
void MainMenuScene::drawMainMenu() {
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);

    // ---- title ----
    std::string title = "=== ISOLATION CHESS ===";
    int titleRow = maxY / 2 - 5;
    int titleCol = (maxX - (int)title.length()) / 2;

    attron(A_BOLD);
    mvaddstr(titleRow, titleCol, title.c_str());
    attroff(A_BOLD);

    // ---- menu options ----
    std::vector<std::string> options = {
        "Human vs Human",
        "Human vs AI",
        "Quit"
    };

    for (int i = 0; i < (int)options.size(); i++) {
        int row = titleRow + 3 + i;

        // put a little arrow next to the highlighted one
        std::string label = (i == m_selectedOption ? "> " : "  ") + options[i];
        int col = (maxX - (int)label.length()) / 2;

        if (i == m_selectedOption) {
            // A_REVERSE flips foreground/background so it looks highlighted
            attron(A_REVERSE);
            mvaddstr(row, col, label.c_str());
            attroff(A_REVERSE);
        } else {
            mvaddstr(row, col, label.c_str());
        }
    }

    // ---- controls hint ----
    std::string hint = "Arrow keys / WASD to navigate, Enter to select, Q to quit";
    int hintCol = (maxX - (int)hint.length()) / 2;
    mvaddstr(titleRow + 8, hintCol, hint.c_str());
}

// draws the AI difficulty submenu
void MainMenuScene::drawDifficultyMenu() {
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);

    // ---- submenu title ----
    std::string title = "=== SELECT AI DIFFICULTY ===";
    int titleRow = maxY / 2 - 5;
    int titleCol = (maxX - (int)title.length()) / 2;

    attron(A_BOLD);
    mvaddstr(titleRow, titleCol, title.c_str());
    attroff(A_BOLD);

    // ---- difficulty options with short descriptions ----
    std::vector<std::string> options = {
        "Easy   - Random moves",
        "Medium - Greedy strategy",
        "Hard   - Minimax AI",
        "Back"
    };

    // same drawing logic as the main menu
    for (int i = 0; i < (int)options.size(); i++) {
        int row = titleRow + 3 + i;
        std::string label = (i == m_selectedDifficulty ? "> " : "  ") + options[i];
        int col = (maxX - (int)label.length()) / 2;

        if (i == m_selectedDifficulty) {
            attron(A_REVERSE);
            mvaddstr(row, col, label.c_str());
            attroff(A_REVERSE);
        } else {
            mvaddstr(row, col, label.c_str());
        }
    }

    // ---- hint ----
    std::string hint = "Arrow keys / WASD to navigate, Enter to select, ESC to go back";
    int hintCol = (maxX - (int)hint.length()) / 2;
    mvaddstr(titleRow + 9, hintCol, hint.c_str());
}
*/

#include "ui/main_menu_scene.hpp"
#include "app/app.hpp"

#include <ncurses.h>
#include <string>
#include <vector>

MainMenuScene::MainMenuScene() = default;

void MainMenuScene::handleInput(App &app, int ch)
{
    if (ch == ERR)
        return;

    // Quick quit
    if (!m_inDifficultyMenu && (ch == 'q' || ch == 'Q'))
    {
        app.requestQuit();
        return;
    }

    if (!m_inDifficultyMenu)
    {
        switch (ch)
        {
        case KEY_UP:
        case 'w':
        case 'W':
            if (m_selectedOption > 0)
                m_selectedOption--;
            break;

        case KEY_DOWN:
        case 's':
        case 'S':
            if (m_selectedOption < 2)
                m_selectedOption++;
            break;

        case '\n':
        case '\r':
        case KEY_ENTER:
        case 'c':
        case 'C':
            if (m_selectedOption == 0)
            {
                app.startNewHumanGame();
            }
            else if (m_selectedOption == 1)
            {
                m_inDifficultyMenu = true;
                m_selectedDifficulty = 0;
            }
            else
            {
                app.requestQuit();
            }
            break;
        }
    }
    else
    {
        switch (ch)
        {
        case KEY_UP:
        case 'w':
        case 'W':
            if (m_selectedDifficulty > 0)
                m_selectedDifficulty--;
            break;

        case KEY_DOWN:
        case 's':
        case 'S':
            if (m_selectedDifficulty < 3)
                m_selectedDifficulty++;
            break;

        case 27: // ESC
        case 'x':
        case 'X':
            m_inDifficultyMenu = false;
            break;

        case '\n':
        case '\r':
        case KEY_ENTER:
        case 'c':
        case 'C':
            if (m_selectedDifficulty == 3)
            {
                m_inDifficultyMenu = false;
            }
            else
            {
                AiDifficulty diff =
                    (m_selectedDifficulty == 0) ? AiDifficulty::Easy : (m_selectedDifficulty == 1) ? AiDifficulty::Medium
                                                                                                   : AiDifficulty::Hard;

                app.startNewAiGame(diff);
            }
            break;
        }
    }
}

void MainMenuScene::update(App &app)
{
    (void)app;
}

void MainMenuScene::render(App &app)
{
    (void)app;
    erase();
    if (m_inDifficultyMenu)
        drawDifficultyMenu();
    else
        drawMainMenu();
    refresh();
}

void MainMenuScene::drawMainMenu() const
{
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);

    std::string title = "=== ISOLATION CHESS ===";
    int titleRow = maxY / 2 - 5;
    int titleCol = (maxX - (int)title.length()) / 2;

    attron(A_BOLD);
    mvaddstr(titleRow, titleCol, title.c_str());
    attroff(A_BOLD);

    std::vector<std::string> options = {
        "Human vs Human",
        "Human vs AI",
        "Quit"};

    for (int i = 0; i < (int)options.size(); i++)
    {
        int row = titleRow + 3 + i;
        std::string label = (i == m_selectedOption ? "> " : "  ") + options[i];
        int col = (maxX - (int)label.length()) / 2;

        if (i == m_selectedOption)
            attron(A_REVERSE);
        mvaddstr(row, col, label.c_str());
        if (i == m_selectedOption)
            attroff(A_REVERSE);
    }

    std::string hint = "Arrow keys / WASD, Enter to select, Q to quit";
    int hintCol = (maxX - (int)hint.length()) / 2;
    mvaddstr(titleRow + 8, hintCol, hint.c_str());
}

void MainMenuScene::drawDifficultyMenu() const
{
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);

    std::string title = "=== SELECT AI DIFFICULTY ===";
    int titleRow = maxY / 2 - 5;
    int titleCol = (maxX - (int)title.length()) / 2;

    attron(A_BOLD);
    mvaddstr(titleRow, titleCol, title.c_str());
    attroff(A_BOLD);

    std::vector<std::string> options = {
        "Easy",
        "Medium",
        "Hard",
        "Back"};

    for (int i = 0; i < (int)options.size(); i++)
    {
        int row = titleRow + 3 + i;
        std::string label = (i == m_selectedDifficulty ? "> " : "  ") + options[i];
        int col = (maxX - (int)label.length()) / 2;

        if (i == m_selectedDifficulty)
            attron(A_REVERSE);
        mvaddstr(row, col, label.c_str());
        if (i == m_selectedDifficulty)
            attroff(A_REVERSE);
    }

    std::string hint = "Arrow keys / WASD, Enter to select, ESC to go back";
    int hintCol = (maxX - (int)hint.length()) / 2;
    mvaddstr(titleRow + 9, hintCol, hint.c_str());
}