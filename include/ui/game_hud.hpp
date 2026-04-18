#pragma once
#include <core/coord.hpp>
#include <core/game_state.hpp>
#include <optional>
#include <sessions/match_session.hpp>
#include <sessions/replay_session.hpp>
#include <string>
#include <vector>

#include "core/enums.hpp"

/*
Fully expanded state
Width: 24 && Height: 20
╭─HUD──────────────────╮
│ Turn:       Player 1 │
│ Phase:          Move │
│ Time:     14 seconds │
╰──────────────────────╯
╭─Log──────────────────╮
│                      │
│                      │
│                      │
│                      │
│                      │
│                      │
│                      │
│                      │
│                      │
│                      │
╰──────────────────────╯
╭──────────────────────╮
│ Esc to use command   │
╰──────────────────────╯
*/

class GameHud {
 private:
  Coord m_size;
  Coord m_winPos;
  int linesOfLogScrolled{0};

  int m_tick{0};

  int m_hudHeight{5};
  int m_logHeight{12};
  int m_commandHeight{3};
  bool m_flashOn{true};

  std::string m_command{};
  size_t m_commandCursorPos{0};
  bool m_commandReady{false};

  /*(Frame included:) HUD & command boxes are fixed size (5 & ?? rows)*/
  /*Log can be shrunken down to a min of 3 rows*/
  void drawFrame(bool winFocused);

  /*
  Example of messages:
  Nice to have: Styling capability
  E.g. <YELLOW> colors the next space-separated word

  ...
  │ P1: This is dumb.    │
  │ [!] Invalid command  │
  │ P1: Moved to (19, 10 │
  │ ) after 14s.         │
  │ Goto: Turn 120       │
  ╰──────────────────────╯
  */
  /*Full state access seems unnecessary*/
  void drawMessages(const std::vector<std::string>& msgs);

  /*
  command is just m_command. Shouldn't need extra params.
  Cursor position is available as a member too.
  Initially left aligned; right aligned if too long
  Cursor should flash
  ╭──────────────────────╮
  │ dasmals asddd asdd█a │
  ╰──────────────────────╯
  */
  void drawCommand(bool winFocused);

  /*
  Normal mode:
  ╭─HUD──────────────────╮
  │ Turn:       Player 1 │ -> Player 1: Blue; 2: Red (Turn stays white)
  │ Phase:          Move │ -> Break: Red? Move: Blue? I don't know if it'll look
  good. │ Time:     14 seconds │ -> X seconds: Flashes every 0.75s
  ╰──────────────────────╯
  Extra info for replay mode:
  ╭─HUD──────────────────╮
  │ Turn:       Player 1 │
  │ Phase:          Move │
  │ Progress:     14/123 │ -> Not enough space for time here
  │ Playback speed:   2x │
  │ Autoplay:     Active │ -> Color coded
  ╰──────────────────────╯

  */
  /*Polymorphism*/
  void drawHUD(const MatchSession& session);
  void drawHUD(const ReplaySession& session);

 public:
  /*Default values. Looks good for regular game*/
  GameHud(Coord size = {20, 24}, Coord pos = {0, 48})
      : m_size(size), m_winPos(pos) {};

  void resize(int rows, int cols);

  void moveTo(int rows, int cols);

  /*Overloaded*/
  /*If HUD focused: Key_up scrolls messsages up a line, vice versa*/
  /*I guess this part should also decide how tall each part is going to be*/
  void render(int key, bool winFocused, const MatchSession& session);
  void render(int key, bool winFocused, const ReplaySession& session);

  std::optional<std::string> consumeCommand();
};
