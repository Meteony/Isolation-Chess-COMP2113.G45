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
    src/core/replay_io.cpp \
    src/players/human_player.cpp \
    src/players/ai_player.cpp \
    src/sessions/match_session.cpp \
    src/ui/main_menu_scene.cpp \
    tests/test_main_menu.cpp
	$(CXX) $(CXXFLAGS) -Iinclude $^ -lncurses -o test_main_menu

test_replay_match: \
    src/core/game_state.cpp \
	src/core/game_rules.cpp \
    src/core/replay_io.cpp \
	src/sessions/replay_session.cpp \
	tests/test_replay_match.cpp
	$(CXX) $(CXXFLAGS) -Iinclude $^ -lncurses -o test_replay_session

manual_board_renderer: \
    src/core/game_state.cpp \
    src/core/game_rules.cpp \
    src/players/human_player.cpp \
    src/sessions/match_session.cpp \
    src/sessions/replay_session.cpp\
    src/ui/board_renderer.cpp \
    tests/manual_board_renderer.cpp
	$(CXX) $(CXXFLAGS) -Iinclude $^ \
        -lncursesw \
		-o manual_board_renderer
