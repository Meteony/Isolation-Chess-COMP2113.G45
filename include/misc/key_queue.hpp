#pragma once

#include <ncurses.h>

#include <cctype>
#include <cstddef>
#include <deque>

/*
Use this in place of getch.
Usage:

#include "ui/key_queue.hpp"

KeyQueue input;
while (true){
  int ch = input.nextKeyOrErr();
  ...
}

I did exactly this in Python before.
*/

class KeyQueue {
 public:
  explicit KeyQueue(std::size_t maxQueue = 128, bool collapseRepeats = true)
      : m_maxQueue(maxQueue), m_collapseRepeats(collapseRepeats) {}

  int nextKeyOrErr(WINDOW* win = stdscr) {
    drain(win);

    if (m_queue.empty()) {
      return ERR;
    }

    int key = m_queue.front();
    m_queue.pop_front();
    return key;
  }

  bool empty() const { return m_queue.empty(); }
  std::size_t size() const { return m_queue.size(); }
  void clear() { m_queue.clear(); }

 private:
  void drain(WINDOW* win) {
    int ch = wgetch(win);
    while (ch != ERR) {
      enqueue(ch);
      ch = wgetch(win);
    }
  }

  /*
  static int normalizeKey(int ch) {
    if (ch >= 0 && ch <= 255) {
      return std::tolower(static_cast<unsigned char>(ch));
    }
    return ch;
  }
  */

  void enqueue(int ch) {
    if (!m_collapseRepeats || m_queue.empty() || m_queue.back() != ch) {
      m_queue.push_back(ch);
    }

    while (m_queue.size() > m_maxQueue) {
      m_queue.pop_front();
    }
  }

 private:
  std::deque<int> m_queue;
  std::size_t m_maxQueue;
  bool m_collapseRepeats;
};
