#pragma once
#include <memory>
#include "ui/scene.hpp"

class ReplaySession;

class ReplayScene : public Scene
{
public:
    explicit ReplayScene(std::unique_ptr<ReplaySession> session);
    ~ReplayScene() override; // <-- add this

    void handleInput(App &app, int ch) override;
    void update(App &app) override;
    void render(App &app) override;

private:
    std::unique_ptr<ReplaySession> m_session;
};