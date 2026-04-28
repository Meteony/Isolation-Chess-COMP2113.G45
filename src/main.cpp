#include <ncurses.h>

#include <string>

#include "misc/blizzard_transition.hpp"
#include "misc/settings_io.hpp"
#include "scenes/live_match_scene.hpp"
#include "scenes/main_menu_scene.hpp"
#include "scenes/netplay_scene.hpp"
#include "scenes/replay_scene.hpp"
#include "scenes/scene_common.hpp"

int main() {
  Settings settings = SettingsIO::loadSettings().value_or(Settings{});
  if (!SettingsIO::loadSettings()) {
    SettingsIO::saveSettings(settings);
  }

  scenes::NcursesGuard curses;

  std::string roomCode = "room-123";

  BlizzardEffect* effect = new BlizzardEffect;
  if (!effect) return -1;

  namespace main_menu = scenes::main_menu;

  while (true) {
    main_menu::LaunchRequest req =
        main_menu::runLauncher(settings, roomCode, effect);

    if (req.action != main_menu::MenuAction::Netplay) {
      // Netplay owns its blocking pop-ups and transition timing, so avoid
      // drawing another launcher transition on top of it.
      if (effect) effect->startBlockingTransition();
    }

    switch (req.action) {
      case main_menu::MenuAction::HumanVsHuman:
        scenes::runHumanVsHuman(settings, effect);
        break;
      case main_menu::MenuAction::CpuEasy:
        scenes::runHumanVsCpu(settings, AiDifficulty::Easy, effect);
        break;
      case main_menu::MenuAction::CpuMedium:
        scenes::runHumanVsCpu(settings, AiDifficulty::Medium, effect);
        break;
      case main_menu::MenuAction::CpuHard:
        scenes::runHumanVsCpu(settings, AiDifficulty::Hard, effect);
        break;
      case main_menu::MenuAction::Netplay:
        scenes::runNetplay(settings, roomCode, effect);
        break;
      case main_menu::MenuAction::Replay:
        if (!req.replayPath.empty()) scenes::runReplay(req.replayPath, effect);
        break;
      case main_menu::MenuAction::Exit:
        if (effect) {
          effect->startBlockingTransition();
          for (int i = 0; i < 50; ++i) {
            effect->updateAndDraw();
            refresh();
            napms(kFrameMs);
          }
        }
        return 0;
      default:
        break;
    }
  }

  delete effect;
  endwin();
  std::_Exit(130);
  erase();
  return 0;
}
