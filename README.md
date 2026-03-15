# Isolation Chess Development Documentation

Architecture baseline, staged roadmap, validation criteria, and six-member work split.

---

## 0. Project reference

See: https://scratch.mit.edu/projects/241565591/

## 1. Purpose and scope

This document turns the current design discussion into a team-facing implementation guide. It defines the project structure, clarifies class boundaries, organizes development into feasible stages, and gives concrete validation targets so a new member can start contributing without hidden assumptions.

### Project assumptions
- The game is written in C++.
- Early validation should happen without curses.
- Move and break stay as separate phases because that keeps human play interactive.
- Replay must be built on recorded turn history, not by duplicating live gameplay logic.
- Visual overlays and temporary effects belong to scene/UI state, not to `GameState`.

---

## 2. Gameplay rules baseline

The project currently assumes the following rules:

- Each turn has **two phases**:
  1. **Move phase**: the active player moves to any legal tile in the 3×3 area centered on their current position.
  2. **Break phase**: the active player chooses one valid tile anywhere on the board to break.
- A legal move must land on an intact, unoccupied tile.
- A legal break may target any intact, unoccupied tile on the board.
- A player wins by leaving the opponent with **no legal move** on the opponent’s next turn.
- Standing still is treated as illegal unless the team explicitly decides otherwise and updates the tests.

### Design consequence
Because move and break are separate and meaningful, `MatchSession` should drive the live phase flow explicitly. `GameRules` should validate and apply move and break separately.

---

## 3. Core architecture

### 3.1 Layered model

The project is split into three layers.

#### A. Core engine
These classes define the actual game and rules.
- `Coord`
- `TurnRecord`
- `GameState`
- `GameRules`

#### B. Control layer
These classes drive live play and replay.
- `Player`
- `HumanPlayer`
- `CpuPlayer`
- `MatchSession`
- `ReplaySession`

#### C. UI layer
These classes handle curses and screen logic.
- `Scene`
- `MainMenuScene`
- `LiveGameScene`
- `ReplayScene`
- `BoardRenderer`
- `InputOverlay`
- `VisualEffectsState`
- `App`

### 3.2 High-level relationships

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

### 3.3 Responsibility split

| Component | Owns / does | Must not do |
|---|---|---|
| `GameState` | Board tiles, player positions, side to move, result; pure data accessors | Input, rendering, rule validation, temporary visual feedback |
| `GameRules` | Legal move/break checks, `applyMove`, `applyBreak`, mobility and terminal checks | Turn loop, input handling, rendering |
| `Player` | Provide move/break decisions through a shared interface | Mutate `GameState` directly |
| `HumanPlayer` | Cursor, selection state, keyboard-driven decisions, overlay data | Rule authority, board ownership |
| `CpuPlayer` | Difficulty, delay countdown, automated decisions | Blocking the main loop with `sleep()` |
| `MatchSession` | Own live `GameState` and players; run move -> break -> next turn; record history | Draw UI directly, own menu logic |
| `ReplaySession` | Rebuild replay state from turn history; step, reset, autoplay | Act as a live-match subclass |
| `BoardRenderer` | Draw board and HUD from `GameState` + optional UI structs | Validate gameplay or own scenes |
| `Scene / App` | Top-level input, update, render flow and screen switching | Contain core rule logic |

### 3.4 Recommended file tree

```text
isolation-chess/
├─ include/
│  ├─ core/      coord.hpp, enums.hpp, turn_record.hpp, game_state.hpp, game_rules.hpp
│  ├─ players/   player.hpp, human_player.hpp, cpu_player.hpp
│  ├─ sessions/  match_session.hpp, replay_session.hpp
│  ├─ ui/        input_overlay.hpp, visual_effects_state.hpp, scene.hpp,
│  │             board_renderer.hpp, live_game_scene.hpp, replay_scene.hpp, main_menu_scene.hpp
│  └─ app/       app.hpp
├─ src/
│  ├─ core/      game_state.cpp, game_rules.cpp
│  ├─ players/   human_player.cpp, cpu_player.cpp
│  ├─ sessions/  match_session.cpp, replay_session.cpp
│  ├─ ui/        board_renderer.cpp, live_game_scene.cpp, replay_scene.cpp, main_menu_scene.cpp
│  ├─ app/       app.cpp
│  └─ main.cpp
├─ tests/        test_game_state.cpp, test_game_rules.cpp, test_match_session.cpp,
│                test_human_player.cpp, test_cpu_player.cpp, test_replay_session.cpp, scripted_player.hpp
├─ docs/         gameplay_rules.md, architecture, roadmap, test plan
├─ assets/       sample_replays/
├─ README.md
└─ Makefile
```

---

## 4. Staged development roadmap

The roadmap is intentionally incremental. Each stage adds one layer of abstraction only after the previous layer is stable.

| Stage | Primary work in `./src` | Primary work in `./tests` | Deliverable | Validation / exit criteria |
|---|---|---|---|---|
| **Stage 1** Rules, `GameState`, terminal prototype | `Coord`, enums, `GameState`, `GameRules`, `TurnRecord`, minimal turn flow. Use `std::cin/std::cout` only. | Barebones local 1v1 prototype plus core legality tests. | A playable text-mode match and a stable rule/state baseline. | Two human players can complete a legal full game in a basic terminal. Rule tests pass without curses. |
| **Stage 2** Player abstraction + minimal curses | `Player`, `HumanPlayer`, `CpuPlayer`, `MatchSession` finalized. Minimal `ncurses` render/control loop. | Basic interactive curses test harness for human vs CPU / human vs human. | Full keyboard-controlled gameplay with CPU difficulty and non-blocking delay. | Human vs CPU is fully playable. CPU actions are always legal. App remains responsive. |
| **Stage 3** Scene/App/renderer cleanup | `Scene`, `LiveGameScene`, `BoardRenderer`, `App`, `InputOverlay`, `VisualEffectsState`. | Regression checks to ensure stage-2 behavior still works after refactor. | A proper scene-based application structure with clean rendering boundaries. | The game still behaves correctly, but UI flow and rendering are no longer tangled with gameplay control. |
| **Stage 4** Replay, replay scene, main menu | `ReplaySession`, `ReplayScene`, `MainMenuScene`, replay I/O if needed. | Replay regression tests and scene navigation tests. | Recorded games can be replayed and accessed from a main menu flow. | Replay reproduces live matches exactly. Menu can enter live game and replay without destabilizing the core. |

---

## 5. Stage details and concrete expectations

### 5.1 Stage 1 — core rules and terminal prototype

This stage is the foundation. It should prove that the game is logically correct before any curses complexity is added.

#### In `./src`
- `Coord`
- enums
- `GameState`
- `GameRules`
- `TurnRecord`
- minimal live turn flow (`MatchSession` or equivalent)

#### In `./tests`
- barebones `std::cin/std::cout` 1v1 game
- move legality tests
- break legality tests
- terminal detection tests
- copy and assignment tests for `GameState`

#### Deliverable
- local 1v1 fully playable in terminal text mode
- core tests pass

#### Validation
- no curses required
- both players can finish a full legal game
- winner is detected correctly

**Important note:** Stage 1 should include both manual play and programmatic tests. The manual prototype proves usability; the tests prove correctness.

### 5.2 Stage 2 — `Player` hierarchy and minimal curses gameplay

Once the rules and state are stable, introduce the `Player` abstraction. `HumanPlayer` and `CpuPlayer` must share one interface.

#### In `./src`
- `Player`
- `HumanPlayer`
- `CpuPlayer`
- `MatchSession` finalized if not already done
- CPU difficulty and non-blocking delay logic

#### In `./tests`
- minimal curses loop
- human vs CPU
- human vs human if useful to keep

#### Deliverable
- keyboard-controlled curses gameplay
- human vs CPU with multiple difficulties
- still visually simple

#### Validation
- full playable match in curses
- CPU always outputs legal actions
- delay works without freezing the app

#### Recommended loop sketch

```cpp
while (running) {
    int ch = getch();

    if (current player is human) {
        currentPlayer->handleInput(ch);
    }

    session.update();

    clear();
    draw board from session.state();
    draw side / phase / winner info;
    refresh();
}
```

### 5.3 Stage 3 — formalize scenes and renderer boundaries

This stage is not about new rules. It is about cleaning the architecture so later features can fit naturally.

#### In `./src`
- `Scene`
- `LiveGameScene`
- `BoardRenderer`
- `App`
- `InputOverlay`
- `VisualEffectsState`

#### Deliverable
- proper scene-based game structure
- cleaner renderer separation
- UI logic no longer mixed into core/gameplay logic

#### Validation
- game still behaves exactly like Stage 2
- architecture is cleaner, not just prettier

### 5.4 Stage 4 — replay, replay scene, and menu flow

Replay should be added only after live gameplay is stable. It must reuse recorded turn history and the same rule/state model.

#### In `./src`
- `ReplaySession`
- `ReplayScene`
- `MainMenuScene`
- replay file I/O if needed

#### Deliverable
- finished games can be replayed
- replay scene works
- main menu can enter live game and replay

#### Validation
- replay reproduces live match exactly
- timing metadata only affects pacing, not game correctness

---

## 6. Replay data model clarification

A live game replay can be reconstructed from a sequence of completed turns. The core replay record should stay separate from optional timing metadata.

### Core record

```cpp
struct TurnRecord {
    Side actor;
    Coord moveCoord;
    Coord breakCoord;
};
```

### Optional timing-aware record

```cpp
struct TimedTurnRecord {
    TurnRecord turn;
    int moveThinkTicks;
    int breakThinkTicks;
};
```

### Important correction
Turn history solves **replay** and **logging**. It does **not** replace validation. Validation still comes from `GameRules` operating on the current `GameState`.

---

## 7. Concrete test cases and validation

These tests should exist before the team claims a stage is complete.

| Module | Test ID | Scenario | Expected result |
|---|---|---|---|
| `GameState` | GS-01 | Initialize a 5×5 board | All tiles are `Intact` |
| `GameState` | GS-04 | Copy-construct from a state with broken tiles, then modify original | Copied state remains unchanged |
| `GameRules` | GR-M03 | Attempt move outside 3×3 neighborhood | Move is illegal |
| `GameRules` | GR-B03 | Attempt to break the active player tile | Break is illegal |
| `GameRules` | GR-T02 | Completely surround a player with no legal move | Terminal/mobility check reports no move |
| `MatchSession` | MS-02 | Scripted player returns a legal move | Phase advances from Move to Break |
| `MatchSession` | MS-04 | Legal break after a legal move | `TurnRecord` is added, side switches, phase returns to Move |
| `MatchSession` | MS-06 | Break leaves opponent with no legal move | Session finishes and winner is set |
| `HumanPlayer` | HP-02 | Simulated key input moves cursor | Overlay cursor updates correctly |
| `CpuPlayer` | CP-01 | Delay is 5 ticks, only 4 updates occur | CPU is not ready yet |
| `ReplaySession` | RS-04 | Replay a real match from initial state plus recorded history | Final replay state matches final live state exactly |

---

## 8. Team work split

A clean split for a four-person team would be:
- Core engineer: `GameState`, `GameRules`, state/rule tests
- Session engineer: `Player` base, `HumanPlayer`, `MatchSession`, scripted-player tests
- UI engineer: `Scene`, `App`, `BoardRenderer`, `LiveGameScene`, `MainMenuScene`
- Features engineer: `ReplaySession`, `ReplayScene`, `CpuPlayer`, difficulty tuning, optional extras

---

## 9. Minimum acceptable scope

The project is in good shape once it has:
- correct move legality
- correct break legality
- correct win detection
- playable local match
- stable scene-based curses loop
- replay based on `TurnRecord`
- at least one working CPU level if time permits

Cut stretch features before risking the baseline. The first things to cut are replay preview in the menu, hard CPU, online mode, and advanced effects.

---

## 10. Stage fit and work split for a 6-member team

With 6 members, the safest structure is to split by both stage and specialty. The goal is to let the strongest core people stabilize the rules early, while UI and feature-oriented members scaffold their layers without blocking the core.

| Stage | Best suited member type | Why this stage fits them | Recommended 6-member allocation |
|---|---|---|---|
| **Stage 1** Rules, `GameState`, terminal prototype | Core logic members, careful debuggers, people strongest at correctness and edge cases | This stage is deterministic and rule-heavy. It rewards people who like invariants, board state, legal/illegal cases, and test writing more than visual polish. | **2 members**: Member A — `GameState` + memory management; Member B — `GameRules` + core tests |
| **Stage 2** Player abstraction + minimal curses | Control-flow / OOP members, people who can bridge state and interaction | This stage connects the rules to actual decision-making and turn flow. It suits members who can make human and CPU behavior fit one clean interface. | **2 members**: Member C — `Player` base + `HumanPlayer`; Member D — `MatchSession` + `CpuPlayer` delay/logic |
| **Stage 3** Scene/App/renderer cleanup | UI / architecture members, people comfortable with `ncurses` and display flow | This stage is about structure, rendering boundaries, and a stable input-update-render loop. It suits members who care about clean screen logic and modular UI. | **1 member**: Member E — `Scene` / `App` / `BoardRenderer` / `LiveGameScene` |
| **Stage 4** Replay, replay scene, main menu | Feature / systems member, someone comfortable with history, I/O, and state reconstruction | Replay depends on earlier future-proofing, so this suits someone who can build on stable abstractions without leaking replay logic into live gameplay. | **1 member**: Member F — `ReplaySession` / `ReplayScene` / `MainMenuScene` / replay I/O |

### Parallelism advice for 6 members

In Week 1–2, Members E and F should scaffold non-invasive files such as `Scene` / `App` skeletons, replay headers, docs, Makefile, and test harness support while Members A–D stabilize the core. They should avoid changing `GameRules` or `MatchSession` interfaces until the core is frozen.

### Suggested member-to-workstream map

- **Member A** — `GameState`, dynamic memory, copy semantics, state tests
- **Member B** — `GameRules`, legality checks, terminal-state tests
- **Member C** — `Player` base, `HumanPlayer`, input overlay production
- **Member D** — `MatchSession`, `CpuPlayer`, scripted-player integration tests
- **Member E** — `Scene`, `App`, `BoardRenderer`, `LiveGameScene`, curses loop polish
- **Member F** — `ReplaySession`, `ReplayScene`, `MainMenuScene`, replay serialization or I/O

If one member is noticeably stronger than the others, that person should act as integration lead and review any change that touches shared interfaces.

---

## 11. Recommended implementation order

1. `coord.hpp`, `enums.hpp`, `turn_record.hpp`
2. `game_state.hpp/.cpp`
3. `game_rules.hpp/.cpp`
4. scripted-player tests
5. `match_session.hpp/.cpp`
6. `human_player.hpp/.cpp`
7. `board_renderer.hpp/.cpp`
8. `scene.hpp`, `live_game_scene.hpp/.cpp`, `app.hpp/.cpp`
9. `replay_session.hpp/.cpp`
10. `cpu_player.hpp/.cpp`

This order keeps dependencies sane and lets the team validate the core before adding higher-level UI and replay features.
