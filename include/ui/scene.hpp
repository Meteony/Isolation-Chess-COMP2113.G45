#pragma once

class App;

class Scene
{
public:
    virtual ~Scene() = default;

    // Runs when the scene becomes active.
    virtual void onEnter(App &app) { (void)app; }
    // Runs when the scene is about to be replaced.
    virtual void onExit(App &app) { (void)app; }

    // Input char from ncurses getch(). Can be ERR (-1) when nodelay.
    virtual void handleInput(App &app, int ch) = 0;

    // Updates scene state for one frame.
    virtual void update(App &app) = 0;
    // Draws the scene for one frame.
    virtual void render(App &app) = 0;
};