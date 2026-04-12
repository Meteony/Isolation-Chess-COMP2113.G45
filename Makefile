CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic

manual_curses_match: \
    src/core/game_state.cpp \
    src/core/game_rules.cpp \
    src/core/replay_io.cpp \
    src/players/human_player.cpp \
    src/sessions/match_session.cpp \
    tests/manual_curses_match.cpp
	$(CXX) $(CXXFLAGS) -Iinclude $^ -lncurses -o manual_curses_match


test_main_menu: \
    src/core/game_state.cpp \
    src/core/game_rules.cpp \
    src/players/human_player.cpp \
    src/players/ai_player.cpp \
    src/sessions/match_session.cpp \
    src/ui/main_menu_scene.cpp \
    tests/test_main_menu.cpp
	$(CXX) $(CXXFLAGS) -Iinclude $^ -lncurses -o test_main_menu
