#pragma once
#include <memory>

#include "ui/scene.hpp"
#include "ui/board_renderer.hpp"
#include "ui/input_overlay.hpp"
#include "ui/visual_effects_state.hpp"

#include "players/player.hpp"
#include "core/enums.hpp"

class MatchSession;

class LiveGameScene : public Scene
{
public:
    static std::unique_ptr<LiveGameScene> makeHumanVsHuman(int rows, int cols);
    static std::unique_ptr<LiveGameScene> makeHumanVsAi(int rows, int cols, AiDifficulty difficulty);

    void handleInput(App &app, int ch) override;
    void update(App &app) override;
    void render(App &app) override;

private:
    LiveGameScene() = default;

    std::unique_ptr<Player> m_p1;
    std::unique_ptr<Player> m_p2;
    std::unique_ptr<MatchSession> m_session;

    BoardRenderer m_renderer;
    InputOverlay m_overlay;
    VisualEffectsState m_vfx;
};