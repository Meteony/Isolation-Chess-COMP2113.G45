#pragma once
#include "core/enums.hpp"

// Stores what the player chose from the main menu
// so the caller knows what kind of game to set up
struct MenuChoice {
    enum GameMode {
        HumanVsHuman,
        HumanVsAi,
        Quit
    };

    GameMode mode = GameMode::Quit;
    AiDifficulty difficulty = AiDifficulty::Easy;  // only matters if mode is HumanVsAi
};

// Handles the main menu screen using ncurses.
// Shows game mode options and an AI difficulty submenu.
// Call run() and it blocks until the player picks something.
class MainMenuScene {
public:
    MainMenuScene();

    // Runs the menu loop, returns once the player makes a choice
    MenuChoice run();

private:
    int m_selectedOption;       // which item is highlighted on the main menu
    bool m_inDifficultyMenu;    // whether we're showing the difficulty submenu
    int m_selectedDifficulty;   // which item is highlighted on the difficulty menu

    // draws the main menu screen
    void drawMainMenu();

    // draws the difficulty selection submenu
    void drawDifficultyMenu();
};