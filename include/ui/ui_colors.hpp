#pragma once
#include <ncurses.h>

enum UiColorPair {
  CP_P1 = 1,
  CP_P2 = 2,
  CP_FRAME = 3,
  CP_FRAME_FOCUSED = 4,

  CP_CURSOR_HL = 5,
  CP_PHASE_MOVE = 6,
  CP_PHASE_BREAK = 7,
  CP_ACTIVE = 8,

  CP_MSG_BLACK = 9,
  CP_MSG_RED = 10,
  CP_MSG_GREEN = 11,
  CP_MSG_YELLOW = 12,
  CP_MSG_BLUE = 13,
  CP_MSG_MAGENTA = 14,
  CP_MSG_CYAN = 15,
  CP_MSG_WHITE = 16,

};

inline void ensureUiColorsInitialized() {
  static bool initialized = false;
  if (initialized) return;

  if (has_colors()) {
    start_color();
    use_default_colors();

    (COLORS >= 256) ? init_pair(CP_P1, 202, -1)
                    : init_pair(CP_P1, COLOR_YELLOW, -1);
    init_pair(CP_P2, COLOR_CYAN, -1);

    if (COLORS >= 256) {
      init_pair(CP_FRAME, 246, -1);
    } else
      init_pair(CP_FRAME, COLOR_WHITE, -1);

    if (COLORS >= 256) {
      init_pair(CP_FRAME_FOCUSED, 202, -1);
      // init_pair(CP_FRAME_FOCUSED, COLOR_WHITE, -1);
    } else
      init_pair(CP_FRAME_FOCUSED, COLOR_YELLOW, -1);
    // init_pair(CP_FRAME_FOCUSED, COLOR_WHITE, -1);

    init_pair(CP_CURSOR_HL, COLOR_GREEN, -1);
    init_pair(CP_PHASE_MOVE, COLOR_WHITE, -1);
    (COLORS >= 256) ? init_pair(CP_PHASE_BREAK, COLOR_WHITE, -1)
                    : init_pair(CP_PHASE_BREAK, COLOR_WHITE, -1);
    init_pair(CP_ACTIVE, COLOR_GREEN, -1);

    init_pair(CP_MSG_BLACK, COLOR_BLACK, -1);
    init_pair(CP_MSG_RED, COLOR_RED, -1);
    init_pair(CP_MSG_GREEN, COLOR_GREEN, -1);
    init_pair(CP_MSG_YELLOW, COLOR_YELLOW, -1);
    init_pair(CP_MSG_BLUE, COLOR_BLUE, -1);
    init_pair(CP_MSG_MAGENTA, COLOR_MAGENTA, -1);
    init_pair(CP_MSG_CYAN, COLOR_CYAN, -1);
    init_pair(CP_MSG_WHITE, COLOR_WHITE, -1);
  }

  initialized = true;
}
