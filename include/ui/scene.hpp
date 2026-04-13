#pragma once

class App;

class Scene
{
public:
    virtual ~Scene() = default;

    virtual void onEnter(App &app) { (void)app; }
    virtual void onExit(App &app) { (void)app; }

    // Input char from ncurses getch(). Can be ERR (-1) when nodelay.
    virtual void handleInput(App &app, int ch) = 0;

    virtual void update(App &app) = 0;
    virtual void render(App &app) = 0;
};