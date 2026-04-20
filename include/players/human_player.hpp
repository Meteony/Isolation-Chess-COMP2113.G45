#pragma once
#include "core/enums.hpp"
#include "player.hpp"

class HumanPlayer : public Player {
 private:
  Coord m_cursor{};
  Coord m_pendingMove{};
  Coord m_pendingBreak{};

  bool m_moveReady = false;
  bool m_breakReady = false;
  bool m_hasSelectedMove = false;

 public:
  // Creates a human-controlled player.
  HumanPlayer();
  ~HumanPlayer() override = default;

  // Starts cursor selection for the move phase.
  void beginMovePhase(const GameState& state) override;
  // Starts cursor selection for the break phase.
  void beginBreakPhase(const GameState& state) override;

  // Processes keyboard input against state.
  void update(int ch, const GameState& state) override;

  // Returns true when a move selection is ready.
  bool hasMoveReady() const override;
  // Returns the chosen move and clears that ready state.
  Coord consumeMove() override;

  // Returns true when a break selection is ready.
  bool hasBreakReady() const override;
  // Returns the chosen break and clears that ready state.
  Coord consumeBreak() override;

  // Returns the current cursor position.
  const Coord cursor() const;
};
