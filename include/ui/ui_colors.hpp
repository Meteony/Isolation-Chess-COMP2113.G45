#pragma once
#include <ncurses.h>

enum UiColorPair {
  CP_P1 = 1,
  CP_P2 = 2,
  CP_FRAME_FOCUSED = 3,
  CP_CURSOR_HL = 4,
  CP_PHASE_MOVE = 5,
  CP_PHASE_BREAK = 6,
  CP_ACTIVE = 7
};

inline void ensureUiColorsInitialized() {
  static bool initialized = false;
  if (initialized) return;

  if (has_colors()) {
    start_color();
    use_default_colors();

    init_pair(CP_P1, COLOR_BLUE, -1);
    init_pair(CP_P2, COLOR_RED, -1);

    // Use yellow for focused box outlines, as you wanted.
    init_pair(CP_FRAME_FOCUSED, COLOR_GREEN, -1);

    init_pair(CP_CURSOR_HL, COLOR_GREEN, -1);
    init_pair(CP_PHASE_MOVE, COLOR_BLUE, -1);
    init_pair(CP_PHASE_BREAK, COLOR_RED, -1);
    init_pair(CP_ACTIVE, COLOR_GREEN, -1);
  }

  initialized = true;
}
