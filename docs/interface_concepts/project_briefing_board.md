# Isolation Chess - Project Board, Milestones, and 7-Week Execution Plan

## 1. Project purpose

This document turns the architecture plan into a practical execution board so that:
- a new team member can onboard without hidden context
- work can be split cleanly across team members
- the team can build in the right order
- replay, CPU, and menu preview stay feasible without rewriting the core

The game rules assumed here are:

- Players move first, then break a tile.
- A move may go to any tile within the 3x3 area centered on the player's current position.
- A break may target any valid tile on the board.
- A player wins by leaving the opponent with no legal move.

---

## 2. Core architecture summary

### Design rule
The project should be built in layers:

1. **Game data**
   - `GameState`
   - board, positions, current side, result

2. **Game rules**
   - `GameRules`
   - validates moves and breaks
   - applies legal state changes
   - checks win condition

3. **Decision providers**
   - `Player` base class
   - `HumanPlayer`
   - `CpuPlayer`
   - `OnlinePlayer` later if needed

4. **Session controllers**
   - `MatchSession` for live gameplay
   - `ReplaySession` for recorded playback

5. **Rendering / UI**
   - `BoardRenderer`
   - `Scene`
   - `LiveGameScene`
   - `ReplayScene`
   - `MainMenuScene`

### Core architecture rule
- `GameState` is the source of truth.
- `GameRules` is the only place that decides legality.
- `MatchSession` updates the state through `GameRules`.
- `Player` objects propose actions; they do **not** directly mutate `GameState`.
- `BoardRenderer` only draws.
- Replay reuses the same board/state/rule model instead of duplicating gameplay logic.

---

## 3. Class responsibilities

### `GameState`
Stores:
- board tiles
- player positions
- current side to move
- result / winner

Does **not**:
- read input
- draw
- validate moves
- own player objects

### `GameRules`
Handles:
- `isLegalMove(...)`
- `isLegalBreak(...)`
- `applyMove(...)`
- `applyBreak(...)`
- `hasAnyLegalMove(...)`
- `isTerminal(...)`

Does **not**:
- read keyboard input
- render UI
- own session flow

### `Player`
Provides:
- move selection
- break selection
- phase-specific input/update logic

Derived classes:
- `HumanPlayer`
- `CpuPlayer`
- `OnlinePlayer`

### `MatchSession`
Owns:
- current `GameState`
- two `Player*`
- current turn phase
- history log

Handles:
- live turn flow
- switching from move phase to break phase
- recording completed turns
- win detection

### `ReplaySession`
Owns:
- initial state
- current replay state
- replay history
- current replay index

Handles:
- step forward
- step backward
- reset
- autoplay if desired

### `BoardRenderer`
Draws:
- board tiles
- player positions
- highlights
- status text

Should receive:
- `GameState`
- optional `InputOverlay`

It should **not** depend directly on full `HumanPlayer` internals.

---

## 4. Recommended project board columns

Use one shared board with these columns:

- **Backlog**
- **Ready**
- **In Progress**
- **Review**
- **Blocked**
- **Done**

### Tag system

#### Area tags
- `core`
- `rules`
- `session`
- `ui`
- `render`
- `replay`
- `cpu`
- `online`
- `docs`
- `tests`

#### Priority tags
- `P0` critical
- `P1` high
- `P2` normal
- `P3` optional

#### Owner tags
Use role tags first, then replace with names:
- `@Lead`
- `@Core`
- `@Session`
- `@UI`
- `@Features`

---

## 5. Team roles

### `@Lead`
Best for:
- architecture decisions
- interface freezing
- integration review
- timeline control
- submission prep

### `@Core`
Best for:
- `GameState`
- `GameRules`
- tests
- edge case handling

### `@Session`
Best for:
- `Player` interface
- `HumanPlayer`
- `MatchSession`
- turn flow
- history recording

### `@UI`
Best for:
- curses loop
- renderer
- scenes
- menus
- status display
- controls

### `@Features`
Best for:
- replay
- CPU
- difficulty tuning
- delay behavior
- optional extra features

---

## 6. Milestones

### M1 - Rules and interfaces locked
**Target:** end of Week 1

Done when:
- gameplay rules are documented
- core interfaces are agreed:
  - `GameState`
  - `GameRules`
  - `Player`
  - `MatchSession`
  - `TurnRecord`
- project compiles with stub files

---

### M2 - Playable local core
**Target:** end of Week 2

Done when:
- two local human players can play a full game
- move legality works
- break legality works
- win detection works
- no renderer polish required yet

---

### M3 - Stable UI loop
**Target:** end of Week 3

Done when:
- curses app loop works cleanly
- board renders consistently
- status info is visible
- local gameplay is comfortable to test

---

### M4 - Replay works
**Target:** end of Week 4

Done when:
- completed turns are logged
- replay can step forward
- replay can step backward
- replay can reset
- replay reproduces the same match state evolution

---

### M5 - CPU opponent works
**Target:** end of Week 5

Done when:
- CPU can play full games
- at least easy + medium difficulties exist
- constructor-configured delay is working
- no blocking sleep is used inside the game loop

---

### M6 - Integration and polish
**Target:** end of Week 6

Done when:
- menu navigation works
- replay scene is integrated
- optional replay preview widget works
- UI cleanup and bug fixes are underway

---

### M7 - Submission freeze
**Target:** Week 7

Done when:
- build is stable
- bugs are triaged
- docs are complete
- demo is rehearsed
- scope is frozen

---

## 7. Ready-to-create issues

## Week 1 issues

### IC-01 - Write and approve gameplay rules document
- **Owner:** `@Lead`
- **Tags:** `P0 docs`
- **Depends on:** none
- **Definition of done:**
  - movement rule is written clearly
  - break rule is written clearly
  - win condition is written clearly
  - edge cases are listed
  - team agrees on board size and illegal action handling

### IC-02 - Create compileable skeleton project
- **Owner:** `@Lead`
- **Tags:** `P0 core`
- **Depends on:** IC-01
- **Definition of done:**
  - include/src folder structure exists
  - basic headers exist
  - basic source files exist
  - Makefile builds successfully

### IC-03 - Define core shared types
- **Owner:** `@Core`
- **Tags:** `P0 core`
- **Depends on:** IC-02
- **Definition of done:**
  - `Coord`
  - enums for side / tile / turn phase / status
  - `TurnRecord`
  - consistent naming style

### IC-04 - Define `GameState` interface
- **Owner:** `@Core`
- **Tags:** `P0 core`
- **Depends on:** IC-03
- **Definition of done:**
  - board storage strategy is fixed
  - dynamic memory ownership is clear
  - copy constructor / assignment strategy is decided
  - getter/setter responsibilities are agreed

### IC-05 - Define `Player` and `MatchSession` interfaces
- **Owner:** `@Session`
- **Tags:** `P0 session`
- **Depends on:** IC-01
- **Definition of done:**
  - move phase and break phase are separate
  - session flow is step-based
  - ownership of players is documented
  - no direct `GameState` mutation from players

---

## Week 2 issues

### IC-06 - Implement dynamic board storage in `GameState`
- **Owner:** `@Core`
- **Tags:** `P0 core`
- **Depends on:** IC-04
- **Definition of done:**
  - board allocates correctly
  - destructor frees correctly
  - copy constructor works
  - assignment works
  - indexing is safe

### IC-07 - Implement move legality in `GameRules`
- **Owner:** `@Core`
- **Tags:** `P0 rules`
- **Depends on:** IC-06
- **Definition of done:**
  - 3x3 movement is checked correctly
  - occupied tiles are rejected
  - broken tiles are rejected
  - tests cover corners and borders

### IC-08 - Implement break legality in `GameRules`
- **Owner:** `@Core`
- **Tags:** `P0 rules`
- **Depends on:** IC-06
- **Definition of done:**
  - only valid tiles may be broken
  - broken tiles cannot be broken again
  - occupied tiles cannot be broken
  - tests cover edge cases

### IC-09 - Implement terminal check / opponent mobility check
- **Owner:** `@Core`
- **Tags:** `P0 rules`
- **Depends on:** IC-07 IC-08
- **Definition of done:**
  - `hasAnyLegalMove(...)` works
  - session can detect winner correctly after a break

### IC-10 - Implement `HumanPlayer`
- **Owner:** `@Session`
- **Tags:** `P0 session`
- **Depends on:** IC-05 IC-07 IC-08
- **Definition of done:**
  - cursor movement works
  - move phase selection works
  - break phase selection works
  - player stores only input state, not game ownership

### IC-11 - Implement `MatchSession`
- **Owner:** `@Session`
- **Tags:** `P0 session`
- **Depends on:** IC-07 IC-08 IC-09 IC-10
- **Definition of done:**
  - session starts correctly
  - move and break phases advance correctly
  - turns switch correctly
  - result is set correctly

### IC-12 - Add rough text-only playable mode
- **Owner:** `@Session`
- **Tags:** `P1 session`
- **Depends on:** IC-11
- **Definition of done:**
  - two players can finish a full game even before polished UI exists

---

## Week 3 issues

### IC-13 - Implement `BoardRenderer`
- **Owner:** `@UI`
- **Tags:** `P1 ui render`
- **Depends on:** IC-11
- **Definition of done:**
  - board shows intact/broken tiles
  - both players show correctly
  - selected cursor position is visible
  - phase information can be displayed

### IC-14 - Add `InputOverlay` abstraction
- **Owner:** `@UI`
- **Tags:** `P1 render`
- **Depends on:** IC-10 IC-13
- **Definition of done:**
  - renderer accepts overlay data instead of full human player object
  - overlay can show cursor and selected move

### IC-15 - Implement `Scene` base and `App` loop
- **Owner:** `@UI`
- **Tags:** `P1 ui`
- **Depends on:** IC-13
- **Definition of done:**
  - app loop is update/render based
  - current scene can handle input
  - no monolithic blocking gameplay function

### IC-16 - Implement `LiveGameScene`
- **Owner:** `@UI`
- **Tags:** `P1 ui`
- **Depends on:** IC-11 IC-13 IC-15
- **Definition of done:**
  - local gameplay works entirely through curses scene
  - status text and prompts are visible

---

## Week 4 issues

### IC-17 - Add turn history recording
- **Owner:** `@Session`
- **Tags:** `P1 replay`
- **Depends on:** IC-11
- **Definition of done:**
  - every completed turn stores:
    - actor
    - move tile
    - broken tile

### IC-18 - Implement `ReplaySession`
- **Owner:** `@Features`
- **Tags:** `P1 replay`
- **Depends on:** IC-17
- **Definition of done:**
  - reset works
  - step forward works
  - step backward works
  - rebuild from history works

### IC-19 - Implement `ReplayScene`
- **Owner:** `@Features`
- **Tags:** `P1 replay`
- **Depends on:** IC-18 IC-13 IC-15
- **Definition of done:**
  - recorded game can be viewed in its own scene
  - replay uses shared renderer

### IC-20 - Create sample replay files / match logs
- **Owner:** `@Features`
- **Tags:** `P2 replay docs`
- **Depends on:** IC-18
- **Definition of done:**
  - at least 2 known-good replay samples exist for testing

---

## Week 5 issues

### IC-21 - Implement easy CPU
- **Owner:** `@Features`
- **Tags:** `P1 cpu`
- **Depends on:** IC-11
- **Definition of done:**
  - CPU can choose legal move and legal break
  - easy mode may be mostly random but valid

### IC-22 - Implement non-blocking decision delay
- **Owner:** `@Features`
- **Tags:** `P1 cpu`
- **Depends on:** IC-21
- **Definition of done:**
  - delay is set through constructor
  - CPU waits through update countdown
  - app remains responsive
  - no blocking `sleep()` inside gameplay loop

### IC-23 - Implement medium CPU heuristic
- **Owner:** `@Features`
- **Tags:** `P2 cpu`
- **Depends on:** IC-21 IC-22
- **Definition of done:**
  - medium CPU plays noticeably better than easy
  - heuristic is documented briefly

### IC-24 - Expose difficulty configuration cleanly
- **Owner:** `@Features`
- **Tags:** `P2 cpu`
- **Depends on:** IC-21 IC-23
- **Definition of done:**
  - CPU difficulty is chosen at object construction
  - delay values are configurable

---

## Week 6 issues

### IC-25 - Implement `MainMenuScene`
- **Owner:** `@UI`
- **Tags:** `P2 ui`
- **Depends on:** IC-15
- **Definition of done:**
  - menu can route to play / replay / quit

### IC-26 - Integrate replay into menu flow
- **Owner:** `@UI`
- **Tags:** `P2 ui replay`
- **Depends on:** IC-19 IC-25
- **Definition of done:**
  - replay scene can be entered from menu
  - user can return cleanly

### IC-27 - Add replay preview widget to main menu
- **Owner:** `@UI`
- **Tags:** `P3 stretch-goal`
- **Depends on:** IC-18 IC-25
- **Definition of done:**
  - menu can show non-blocking replay preview
  - menu remains responsive

### IC-28 - Optional hard CPU or optional online stub
- **Owner:** `@Features`
- **Tags:** `P3 stretch-goal`
- **Depends on:** IC-24
- **Definition of done:**
  - feature is isolated and does not destabilize core game

---

## Week 7 issues

### IC-29 - Bug triage and fix pass
- **Owner:** `@Lead`
- **Tags:** `P0 tests`
- **Depends on:** all core milestones
- **Definition of done:**
  - bug list reviewed
  - blockers fixed
  - stretch goals cut if needed

### IC-30 - Documentation pass
- **Owner:** `@Lead`
- **Tags:** `P1 docs`
- **Depends on:** major systems complete
- **Definition of done:**
  - architecture brief updated
  - setup instructions updated
  - controls documented
  - known limitations documented

### IC-31 - Submission and demo prep
- **Owner:** `@Lead`
- **Tags:** `P0 docs`
- **Depends on:** IC-29 IC-30
- **Definition of done:**
  - build instructions verified
  - demo flow rehearsed
  - presenters know who explains what

---

## 8. Week-by-week timeline

## Week 1 - Freeze the architecture
### Goal
Lock gameplay rules and core interfaces.

### Deliverables
- rules doc
- compileable project skeleton
- agreed interfaces

### Main owners
- `@Lead`
- `@Core`
- `@Session`

### Review gate
No one starts major implementation until the interfaces are clear.

---

## Week 2 - Build the live playable core
### Goal
Get a rough but working local game.

### Deliverables
- `GameState`
- `GameRules`
- `HumanPlayer`
- `MatchSession`
- rough text-mode gameplay

### Review gate
Two people should be able to complete a full legal match.

---

## Week 3 - Build the real UI loop
### Goal
Move from rough logic test mode to stable scene-based terminal gameplay.

### Deliverables
- renderer
- app loop
- live game scene
- status display

### Review gate
Input, turn flow, and rendering should feel stable enough for repeated manual testing.

---

## Week 4 - Add replay
### Goal
Make game history reusable.

### Deliverables
- turn logging
- replay session
- replay scene
- sample replays

### Review gate
Replay should reproduce the same match progression as live play.

---

## Week 5 - Add CPU
### Goal
Create a believable opponent.

### Deliverables
- easy CPU
- medium CPU
- constructor-based difficulty
- non-blocking delay

### Review gate
CPU can finish a match legally and the delay feels intentional.

---

## Week 6 - Integrate and polish
### Goal
Complete menu flow and high-value extras.

### Deliverables
- main menu
- replay integration
- optional preview widget
- optional hard CPU / online stub only if safe

### Review gate
Project should look like one coherent application.

---

## Week 7 - Freeze and submit
### Goal
Do not add risky new features.

### Deliverables
- bug fix pass
- docs update
- demo rehearsal
- final submission package

### Review gate
Anything unstable gets cut.

---

## 9. New member onboarding checklist

A new member should do this in order:

1. Read the rules document.
2. Read the architecture summary.
3. Build the project locally.
4. Run the current playable build.
5. Trace one full turn through:
   - `HumanPlayer`
   - `MatchSession`
   - `GameRules`
   - `BoardRenderer`
6. Pick one bounded issue.
7. Check dependencies before editing shared interfaces.
8. Work in a feature branch.
9. Demo the result before merge.

---

## 10. Team workflow rules

- One owner per issue.
- Shared interface changes must be announced before merge.
- `GameRules` is the source of rule truth.
- Players do not directly mutate `GameState`.
- Renderer does not decide legality.
- Replay should reuse history and state, not duplicate gameplay code.
- Protect Week 7 from scope creep.

---

## 11. First-day task for a brand new teammate

If someone joins today, assign this exact path:

### Reading
- rules doc
- architecture doc
- this board

### Hands-on
- build the project
- run one match
- inspect `GameState`, `GameRules`, `MatchSession`

### First safe issue choices
- tests
- renderer cleanup
- replay controls
- CPU tuning
- docs update

Avoid giving a new member a broad interface rewrite as their first task.

---

## 12. Recommended file ownership map

### `@Core`
- `game_state.*`
- `game_rules.*`
- tests for legality and terminal checks

### `@Session`
- `player.*`
- `human_player.*`
- `match_session.*`

### `@UI`
- `board_renderer.*`
- `scene.*`
- `live_game_scene.*`
- `main_menu_scene.*`
- `app.*`

### `@Features`
- `replay_session.*`
- `replay_scene.*`
- `cpu_player.*`
- optional extras

### `@Lead`
- docs
- integration
- milestone tracking
- final review

---

## 13. Minimum success definition

The project is successful even if stretch goals are cut, as long as this works:

- local two-player gameplay
- correct move and break legality
- correct win condition
- stable curses UI
- replay of recorded matches
- at least one working CPU level
- clean documentation and demo flow

Stretch goals should never threaten that baseline.
