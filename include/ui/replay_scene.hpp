#pragma once
#include <memory>
#include "ui/scene.hpp"

class ReplaySession;

class ReplayScene : public Scene
{
public:
    // Creates a replay scene from session.
    explicit ReplayScene(std::unique_ptr<ReplaySession> session);
    // Destroys the replay scene.
    ~ReplayScene() override;

    // Handles one input step using ch.
    void handleInput(App &app, int ch) override;
    // Updates replay state for one frame.
    void update(App &app) override;
    // Draws the replay scene.
    void render(App &app) override;

private:
    std::unique_ptr<ReplaySession> m_session;
};