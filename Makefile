CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic

manual_curses_match: \
    src/core/game_state.cpp \
    src/core/game_rules.cpp \
    src/players/human_player.cpp \
    src/sessions/match_session.cpp \
    tests/manual_curses_match.cpp
	$(CXX) $(CXXFLAGS) -Iinclude $^ -lncurses -o manual_curses_match
