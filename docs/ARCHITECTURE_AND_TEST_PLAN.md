# Isolation Chess — Architecture and Test Plan

## 1. Project summary

This project is a terminal-based Isolation Chess game written in C++ with curses.

### Core gameplay rules
- Each turn has **two separate phases**:
  1. **Move phase**: the active player moves to any legal tile in the 3×3 area centered on their current position.
  2. **Break phase**: the active player chooses one valid tile anywhere on the board to break.
- Broken tiles cannot be occupied or moved onto.
- A player wins by leaving the opponent with **no legal move**.

### Key design decisions
- Move and break remain separate phases for human interactivity.
- Replay is separate from live match control.
- CPU delay is non-blocking and handled inside `CpuPlayer`.
- Rendering uses lightweight UI overlay/effects structs rather than full gameplay objects.

---

## 2. Core architecture

The project is split into three layers:

### A. Core engine
These classes define the actual game and rules.

- `Coord`
- `TurnRecord`
- `GameState`
- `GameRules`

### B. Control layer
These classes drive live play and replay.

- `Player`
- `HumanPlayer`
- `CpuPlayer`
- `MatchSession`
- `ReplaySession`

### C. UI layer
These classes handle curses and screen logic.

- `Scene`
- `MainMenuScene`
- `LiveGameScene`
- `ReplayScene`
- `BoardRenderer`
- `InputOverlay`
- `VisualEffectsState`
- `App`

---

## 3. High-level relationships

```text
App
  owns current Scene

Scene
  MainMenuScene
  LiveGameScene -> owns MatchSession -> owns GameState + Player*
  ReplayScene   -> owns ReplaySession

MatchSession
  uses GameRules
  records TurnRecord history

ReplaySession
  replays TurnRecord history using GameRules and GameState

BoardRenderer
  draws GameState + optional InputOverlay + optional VisualEffectsState
```

---

## 4. Class responsibilities

## `Coord`
Simple coordinate pair.

### Owns
- `row`
- `col`

### Notes
- Used throughout the project for tile addressing.

---

## `TurnRecord`
Represents one completed turn.

### Owns
- `actor`
- `moveCoord`
- `breakCoord`

### Notes
- Used for replay.
- Stored only after a full move+break is completed.

---

## `GameState`
Pure gameplay data container.

### Owns
- board dimensions
- dynamic tile array
- player positions
- side to move
- session status
- winner

### Does
- provides getters/setters for board and positions
- provides bounds/index helpers

### Does not
- validate rules
- read input
- draw UI
- contain temporary visual effects

---

## `GameRules`
Rule authority for legality and state transitions.

### Own methods
- `isLegalMove(...)`
- `isLegalBreak(...)`
- `applyMove(...)`
- `applyBreak(...)`
- `hasAnyLegalMove(...)`
- `isTerminal(...)`

### Does
- validate move phase
- validate break phase
- apply legal changes to `GameState`
- check terminal/mobility conditions

### Does not
- own the turn loop
- ask players for decisions
- render feedback

---

## `Player`
Abstract decision interface.

### Required methods
- `beginMovePhase(...)`
- `beginBreakPhase(...)`
- `handleInput(...)`
- `update(...)`
- `hasMoveReady()`
- `consumeMove()`
- `hasBreakReady()`
- `consumeBreak()`

### Notes
- A player does **not** mutate `GameState` directly.
- It only provides decisions.

---

## `HumanPlayer`
Interactive player input logic.

### Owns
- cursor position
- selected move
- selected break
- internal readiness flags
- local phase-related input state

### Does
- respond to keyboard input
- prepare move or break decisions
- expose `InputOverlay` for rendering

---

## `CpuPlayer`
Automated player logic.

### Owns
- difficulty level
- move delay
- break delay
- countdown timer
- pending move/break decisions

### Does
- choose legal move and break
- delay output in a non-blocking way
- expose move/break readiness through the same interface as `HumanPlayer`

---

## `MatchSession`
Live gameplay controller.

### Owns
- live `GameState`
- `Player* p1`
- `Player* p2`
- current phase
- pending move
- dynamic `TurnRecord` history

### Does
- drive move and break phases
- send input to current player
- ask current player to update
- validate decisions through `GameRules`
- apply legal changes to `GameState`
- record history
- switch turns
- detect game over

### Does not
- draw UI directly
- own menu logic

---

## `ReplaySession`
Replay controller.

### Owns
- initial `GameState`
- current replay `GameState`
- replay history copy
- replay index
- autoplay state

### Does
- rebuild state from recorded turns
- step forward
- step backward
- reset replay
- support replay screen or preview widget

### Important rule
- Replay is **not** a subclass of `MatchSession`.

---

## `InputOverlay`
Small render-only struct.

### Owns
- cursor
- selected move marker
- current visible phase

### Purpose
- pass only UI-relevant interaction state to renderer

### Does not
- own logic
- affect `GameState`

---

## `VisualEffectsState`
Temporary render-only feedback state.

### Owns examples
- invalid flash
- flash tile position
- effect timer

### Purpose
- scene-local temporary feedback

### Does not
- affect gameplay rules
- belong in `GameState`

---

## `BoardRenderer`
Board/HUD rendering helper.

### Does
- draw board
- draw players
- draw HUD
- draw optional overlay/effects

### Takes
- `WINDOW*`
- `const GameState&`
- optional `InputOverlay`
- optional `VisualEffectsState`

### Does not
- own screens
- validate moves
- control game flow

---

## `Scene`
UI screen abstraction.

### Required methods
- `handleInput(int ch)`
- `update()`
- `render(WINDOW* win)`

### Derived scenes
- `MainMenuScene`
- `LiveGameScene`
- `ReplayScene`

---

## `LiveGameScene`
Live game screen.

### Owns
- `MatchSession`
- scene-local visual effects

### Does
- forwards input to session
- updates live match
- gathers overlay from current human player if needed
- renders through `BoardRenderer`

---

## `ReplayScene`
Replay screen.

### Owns
- `ReplaySession`

### Does
- handles replay controls
- renders replay state through `BoardRenderer`

---

## `MainMenuScene`
Main menu screen.

### Owns
- menu selection state
- optional preview replay later if desired

---

## `App`
Top-level application object.

### Owns
- current `Scene*`

### Main loop
1. poll input
2. `currentScene->handleInput(ch)`
3. `currentScene->update()`
4. `currentScene->render(win)`

---

## 5. Header files to implement

## `coord.hpp`
```cpp
#pragma once

struct Coord {
    int row = 0;
    int col = 0;

    bool operator==(const Coord& other) const {
        return row == other.row && col == other.col;
    }

    bool operator!=(const Coord& other) const {
        return !(*this == other);
    }
};
```

## `enums.hpp`
```cpp
#pragma once

enum class Side {
    Player1,
    Player2
};

enum class TileState {
    Intact,
    Broken
};

enum class TurnPhase {
    Move,
    Break,
    Finished
};

enum class SessionStatus {
    Running,
    Finished
};

enum class CpuDifficulty {
    Easy,
    Medium,
    Hard
};
```

## `turn_record.hpp`
```cpp
#pragma once
#include "coord.hpp"
#include "enums.hpp"

struct TurnRecord {
    Side actor;
    Coord moveCoord;
    Coord breakCoord;
};
```

## `game_state.hpp`
```cpp
#pragma once
#include "coord.hpp"
#include "enums.hpp"

class GameState {
private:
    int m_rows;
    int m_cols;
    TileState* m_tiles;
    Coord m_p1Pos;
    Coord m_p2Pos;
    Side m_sideToMove;
    SessionStatus m_status;
    Side m_winner;

public:
    GameState(int rows, int cols);
    GameState(const GameState& other);
    GameState& operator=(const GameState& other);
    ~GameState();

    int rows() const;
    int cols() const;

    bool inBounds(Coord c) const;
    int index(Coord c) const;

    TileState tileAt(Coord c) const;
    void setTile(Coord c, TileState state);

    Coord playerPos(Side side) const;
    void setPlayerPos(Side side, Coord pos);

    Side sideToMove() const;
    void setSideToMove(Side side);

    SessionStatus status() const;
    void setStatus(SessionStatus status);

    Side winner() const;
    void setWinner(Side side);
};
```

## `game_rules.hpp`
```cpp
#pragma once
#include "game_state.hpp"

class GameRules {
public:
    static bool isLegalMove(const GameState& state, Side side, Coord dst);
    static bool isLegalBreak(const GameState& state, Coord tile);

    static void applyMove(GameState& state, Side side, Coord dst);
    static void applyBreak(GameState& state, Coord tile);

    static bool hasAnyLegalMove(const GameState& state, Side side);
    static bool isTerminal(const GameState& state);
};
```

## `player.hpp`
```cpp
#pragma once
#include "coord.hpp"
#include "game_state.hpp"

class Player {
public:
    virtual ~Player() = default;

    virtual void beginMovePhase(const GameState& state) = 0;
    virtual void beginBreakPhase(const GameState& state) = 0;

    virtual void handleInput(int ch) = 0;
    virtual void update(const GameState& state) = 0;

    virtual bool hasMoveReady() const = 0;
    virtual Coord consumeMove() = 0;

    virtual bool hasBreakReady() const = 0;
    virtual Coord consumeBreak() = 0;
};
```

## `input_overlay.hpp`
```cpp
#pragma once
#include "coord.hpp"
#include "enums.hpp"

struct InputOverlay {
    bool visible = false;
    Coord cursor{};

    bool hasSelectedMove = false;
    Coord selectedMove{};

    TurnPhase phase = TurnPhase::Move;
};
```

## `visual_effects_state.hpp`
```cpp
#pragma once
#include "coord.hpp"

struct VisualEffectsState {
    bool invalidFlashActive = false;
    Coord invalidFlashTile{};
    int invalidFlashTicks = 0;
};
```

## `human_player.hpp`
```cpp
#pragma once
#include "player.hpp"
#include "input_overlay.hpp"
#include "enums.hpp"

class HumanPlayer : public Player {
private:
    Coord m_cursor{};
    Coord m_pendingMove{};
    Coord m_pendingBreak{};

    bool m_moveReady = false;
    bool m_breakReady = false;
    bool m_hasSelectedMove = false;

    TurnPhase m_phase = TurnPhase::Move;

public:
    HumanPlayer();
    ~HumanPlayer() override = default;

    void beginMovePhase(const GameState& state) override;
    void beginBreakPhase(const GameState& state) override;

    void handleInput(int ch) override;
    void update(const GameState& state) override;

    bool hasMoveReady() const override;
    Coord consumeMove() override;

    bool hasBreakReady() const override;
    Coord consumeBreak() override;

    InputOverlay makeOverlay() const;
};
```

## `cpu_player.hpp`
```cpp
#pragma once
#include "player.hpp"
#include "enums.hpp"

class CpuPlayer : public Player {
private:
    CpuDifficulty m_difficulty;
    TurnPhase m_phase = TurnPhase::Move;

    int m_moveDelayTicks = 0;
    int m_breakDelayTicks = 0;
    int m_ticksRemaining = 0;

    bool m_moveReady = false;
    bool m_breakReady = false;

    Coord m_pendingMove{};
    Coord m_pendingBreak{};

private:
    Coord chooseMove(const GameState& state) const;
    Coord chooseBreak(const GameState& state) const;

public:
    CpuPlayer(CpuDifficulty difficulty, int moveDelayTicks, int breakDelayTicks);
    ~CpuPlayer() override = default;

    void beginMovePhase(const GameState& state) override;
    void beginBreakPhase(const GameState& state) override;

    void handleInput(int ch) override;
    void update(const GameState& state) override;

    bool hasMoveReady() const override;
    Coord consumeMove() override;

    bool hasBreakReady() const override;
    Coord consumeBreak() override;
};
```

## `match_session.hpp`
```cpp
#pragma once
#include "coord.hpp"
#include "enums.hpp"
#include "game_state.hpp"
#include "player.hpp"
#include "turn_record.hpp"

class MatchSession {
private:
    GameState m_state;
    Player* m_p1;
    Player* m_p2;

    TurnPhase m_phase = TurnPhase::Move;
    bool m_hasPendingMove = false;
    Coord m_pendingMove{};

    TurnRecord* m_history = nullptr;
    int m_historySize = 0;
    int m_historyCapacity = 0;

private:
    Player* currentPlayer();
    const Player* currentPlayer() const;

    void beginTurn();
    void ensureHistoryCapacity();
    void pushHistory(const TurnRecord& record);

public:
    MatchSession(int rows, int cols, Player* p1, Player* p2);
    MatchSession(const MatchSession&) = delete;
    MatchSession& operator=(const MatchSession&) = delete;
    ~MatchSession();

    void handleInput(int ch);
    void update();

    const GameState& state() const;
    TurnPhase phase() const;

    const TurnRecord* history() const;
    int historySize() const;

    bool isFinished() const;
};
```

## `replay_session.hpp`
```cpp
#pragma once
#include "game_state.hpp"
#include "turn_record.hpp"

class ReplaySession {
private:
    GameState m_initialState;
    GameState m_currentState;

    TurnRecord* m_history = nullptr;
    int m_historySize = 0;

    int m_currentIndex = 0;
    bool m_autoPlay = false;

private:
    void rebuildToIndex(int index);

public:
    ReplaySession(const GameState& initialState,
                  const TurnRecord* history,
                  int historySize);

    ReplaySession(const ReplaySession&) = delete;
    ReplaySession& operator=(const ReplaySession&) = delete;
    ~ReplaySession();

    void update();
    void stepForward();
    void stepBackward();
    void reset();

    void setAutoPlay(bool enabled);
    bool autoPlay() const;

    const GameState& state() const;
    int currentIndex() const;
    int historySize() const;
};
```

## `scene.hpp`
```cpp
#pragma once
#include <ncurses.h>

class Scene {
public:
    virtual ~Scene() = default;

    virtual void handleInput(int ch) = 0;
    virtual void update() = 0;
    virtual void render(WINDOW* win) = 0;
};
```

## `board_renderer.hpp`
```cpp
#pragma once
#include <ncurses.h>
#include "game_state.hpp"
#include "input_overlay.hpp"
#include "visual_effects_state.hpp"
#include "enums.hpp"

class BoardRenderer {
public:
    static void drawBoard(WINDOW* win,
                          const GameState& state,
                          const InputOverlay* overlay = nullptr,
                          const VisualEffectsState* fx = nullptr);

    static void drawStatusBar(WINDOW* win,
                              const GameState& state,
                              TurnPhase phase);
};
```

## `live_game_scene.hpp`
```cpp
#pragma once
#include "scene.hpp"
#include "match_session.hpp"
#include "board_renderer.hpp"
#include "visual_effects_state.hpp"

class LiveGameScene : public Scene {
private:
    MatchSession* m_match;
    VisualEffectsState m_fx{};

public:
    explicit LiveGameScene(MatchSession* match);
    LiveGameScene(const LiveGameScene&) = delete;
    LiveGameScene& operator=(const LiveGameScene&) = delete;
    ~LiveGameScene() override;

    void handleInput(int ch) override;
    void update() override;
    void render(WINDOW* win) override;
};
```

## `replay_scene.hpp`
```cpp
#pragma once
#include "scene.hpp"
#include "replay_session.hpp"
#include "board_renderer.hpp"

class ReplayScene : public Scene {
private:
    ReplaySession* m_replay;

public:
    explicit ReplayScene(ReplaySession* replay);
    ReplayScene(const ReplayScene&) = delete;
    ReplayScene& operator=(const ReplayScene&) = delete;
    ~ReplayScene() override;

    void handleInput(int ch) override;
    void update() override;
    void render(WINDOW* win) override;
};
```

## `main_menu_scene.hpp`
```cpp
#pragma once
#include "scene.hpp"

class MainMenuScene : public Scene {
private:
    int m_selectedIndex = 0;

public:
    MainMenuScene() = default;
    ~MainMenuScene() override = default;

    void handleInput(int ch) override;
    void update() override;
    void render(WINDOW* win) override;
};
```

## `app.hpp`
```cpp
#pragma once
#include "scene.hpp"

class App {
private:
    Scene* m_currentScene = nullptr;

public:
    App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    ~App();

    void setScene(Scene* scene);
    void run();
};
```

---

## 6. Concrete test plan

These tests should be implemented as early as possible.

## A. `GameState` tests

### GS-01 — board initializes intact
Setup:
- create `GameState(5, 5)`

Expected:
- all tiles are `TileState::Intact`

### GS-02 — bounds checking
Check:
- `(0,0)` valid
- `(4,4)` valid
- `(-1,0)` invalid
- `(0,5)` invalid

### GS-03 — tile read/write
Action:
- set `(2,3)` to `Broken`

Expected:
- reading `(2,3)` returns `Broken`

### GS-04 — copy constructor deep copy
Setup:
- break tiles in original
- copy construct new state
- modify original after copy

Expected:
- copied state does not change

### GS-05 — assignment operator deep copy
Setup:
- assign state A to state B
- mutate A afterward

Expected:
- B stays unchanged

---

## B. `GameRules` move tests

Assume Player1 is at `(2,2)` on a 5×5 board.

### GR-M01 — orthogonal adjacent move legal
Move to `(2,3)`

Expected:
- legal

### GR-M02 — diagonal adjacent move legal
Move to `(1,1)`

Expected:
- legal

### GR-M03 — move outside 3×3 illegal
Move to `(2,4)`

Expected:
- illegal

### GR-M04 — off-board move illegal
From `(0,0)` move to `(-1,0)`

Expected:
- illegal

### GR-M05 — moving onto broken tile illegal
Break `(2,3)` first

Expected:
- move to `(2,3)` is illegal

### GR-M06 — moving onto opponent tile illegal
Put opponent at `(2,3)`

Expected:
- move to `(2,3)` is illegal

### GR-M07 — staying still illegal
Move to current tile `(2,2)`

Expected:
- illegal

---

## C. `GameRules` break tests

### GR-B01 — breaking empty intact tile legal
Break `(4,4)`

Expected:
- legal

### GR-B02 — breaking already broken tile illegal
Break `(4,4)` twice

Expected:
- second break illegal

### GR-B03 — breaking own occupied tile illegal
Break current player tile

Expected:
- illegal

### GR-B04 — breaking opponent occupied tile illegal
Break opponent tile

Expected:
- illegal

### GR-B05 — breaking off-board tile illegal
Break `(6,6)` on 5×5 board

Expected:
- illegal

---

## D. `GameRules` apply tests

### GR-A01 — applyMove changes player position
Move Player1 from `(2,2)` to `(2,3)`

Expected:
- Player1 position becomes `(2,3)`
- tile states unchanged

### GR-A02 — applyBreak marks tile broken
Break `(4,4)`

Expected:
- tile becomes `Broken`

---

## E. terminal-state tests

### GR-T01 — one legal move means not terminal
Set up board so player has exactly one legal move.

Expected:
- `hasAnyLegalMove(...) == true`

### GR-T02 — zero legal moves means terminal
Surround a player completely.

Expected:
- `hasAnyLegalMove(...) == false`
- terminal logic detects game over

---

## F. `MatchSession` tests

Use a scripted fake player for these.

### MS-01 — session starts in move phase
Expected:
- `phase() == TurnPhase::Move`

### MS-02 — legal move advances to break phase
Setup:
- scripted player returns legal move

Expected after update:
- move applied
- phase becomes `Break`

### MS-03 — illegal move does not advance
Setup:
- scripted player returns illegal move

Expected:
- state unchanged
- phase still `Move`

### MS-04 — legal break finalizes turn
Setup:
- move phase already completed
- scripted player returns legal break

Expected:
- break applied
- history size increments
- side switches
- phase returns to `Move`

### MS-05 — illegal break does not finalize turn
Setup:
- break phase active
- scripted player returns illegal break

Expected:
- history unchanged
- phase stays `Break`

### MS-06 — terminal break ends session
Setup:
- arrange board so that break leaves opponent with no legal move

Expected:
- session finished
- correct winner set
- phase becomes `Finished`

---

## G. `HumanPlayer` tests

### HP-01 — move phase resets readiness
Expected:
- `hasMoveReady() == false` after `beginMovePhase`

### HP-02 — cursor movement changes overlay
Simulate input keys.

Expected:
- `makeOverlay().cursor` updates correctly

### HP-03 — confirm on legal move sets move ready
Expected:
- `hasMoveReady() == true`

### HP-04 — break phase sets break readiness on confirm
Expected:
- `hasBreakReady() == true` when valid selection is confirmed

---

## H. `CpuPlayer` tests

### CP-01 — not ready before delay expires
Setup:
- delay 5 ticks

Expected:
- not ready after 4 updates

### CP-02 — ready after delay expires
Expected:
- ready after 5th update

### CP-03 — chosen move always legal
Run across several states.

Expected:
- every consumed move passes `GameRules::isLegalMove`

### CP-04 — chosen break always legal
Run across several states.

Expected:
- every consumed break passes `GameRules::isLegalBreak`

---

## I. `ReplaySession` tests

### RS-01 — reset restores initial state
Replay some turns, then reset.

Expected:
- replay state equals initial state
- index resets to 0

### RS-02 — stepForward reproduces recorded turn
Expected:
- state matches expected board after one turn

### RS-03 — stepBackward reverses progression
Expected:
- state matches earlier replay point

### RS-04 — replay final state matches live final state
Play a real match, build replay from history, replay all turns.

Expected:
- final replay state exactly matches final live state

---

## 7. Useful scripted test helper

```cpp
class ScriptedPlayer : public Player {
private:
    bool m_moveReady = false;
    bool m_breakReady = false;
    Coord m_move{};
    Coord m_break{};

public:
    ScriptedPlayer(Coord move, Coord br) : m_move(move), m_break(br) {}

    void beginMovePhase(const GameState&) override { m_moveReady = true; }
    void beginBreakPhase(const GameState&) override { m_breakReady = true; }

    void handleInput(int) override {}
    void update(const GameState&) override {}

    bool hasMoveReady() const override { return m_moveReady; }
    Coord consumeMove() override { m_moveReady = false; return m_move; }

    bool hasBreakReady() const override { return m_breakReady; }
    Coord consumeBreak() override { m_breakReady = false; return m_break; }
};
```

Use this to test `MatchSession` without curses or real human input.

---

## 8. Recommended implementation order

1. `coord.hpp`, `enums.hpp`, `turn_record.hpp`
2. `game_state.hpp/.cpp`
3. `game_rules.hpp/.cpp`
4. `ScriptedPlayer` tests
5. `match_session.hpp/.cpp`
6. `human_player.hpp/.cpp`
7. `board_renderer.hpp/.cpp`
8. `scene.hpp`, `live_game_scene.hpp/.cpp`, `app.hpp/.cpp`
9. `replay_session.hpp/.cpp`
10. `cpu_player.hpp/.cpp`

---

## 9. Team member starting points

### Core member
Start with:
- `GameState`
- `GameRules`
- state/rules tests

### Session member
Start with:
- `Player`
- `HumanPlayer`
- `MatchSession`
- scripted-player session tests

### UI member
Start with:
- `Scene`
- `BoardRenderer`
- `LiveGameScene`
- `App`

### Feature member
Start later with:
- `ReplaySession`
- `ReplayScene`
- `CpuPlayer`

---

## 10. Minimum success criteria

The project is in good shape once it has:
- correct move legality
- correct break legality
- correct win detection
- playable local match
- stable scene-based curses loop
- replay based on `TurnRecord`
- clear tests for core modules
