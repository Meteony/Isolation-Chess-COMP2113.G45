#pragma once
#include <locale.h>
#include <ncurses.h>

#include <random>
#include <vector>

#include "core/time.hpp"
#include "ui/ui_colors.hpp"

class Pen {
 public:
  int y, x;
  int startTime;
  int endTime;
  bool isDone;

  Pen(int startY, int startX, int start, int end)
      : y(startY), x(startX), startTime(start), endTime(end), isDone(false) {}
};

class BlizzardEffect {
 private:
  std::vector<Pen*> pens;
  int currentTime;
  std::mt19937 rng;

 public:
  BlizzardEffect() : currentTime(0) {
    std::random_device rd;
    rng.seed(rd());
  }

  ~BlizzardEffect() {
    for (Pen* p : pens) {
      delete p;
    }
    pens.clear();
  }

  void fadeLeftToRight(int y0, int x0, int y1, int x1, int sweepTicks,
                       int jitterMax, int t1min, int t1max) {
    std::uniform_int_distribution<int> distJitter(0, jitterMax);
    std::uniform_int_distribution<int> distDuration(t1min, t1max);

    int totalWidth = (x1 - x0 > 0) ? (x1 - x0) : 1;

    for (int y = y0; y <= y1; y += 2) {
      for (int x = x0; x <= x1; x += 4) {
        int baseDelay = ((x - x0) * sweepTicks) / totalWidth;

        int startDelay = baseDelay + distJitter(rng);
        int activeDuration = distDuration(rng);

        int absoluteStart = currentTime + startDelay;
        int absoluteEnd = absoluteStart + activeDuration;

        pens.push_back(new Pen(y, x, absoluteStart, absoluteEnd));
      }
    }
  }

  void updateAndDraw() {
    currentTime++;

    for (auto it = pens.begin(); it != pens.end();) {
      Pen* p = *it;

      if (currentTime >= p->startTime && currentTime < p->endTime) {
        attron(COLOR_PAIR(CP_P1));
        for (int dy = 0; dy < 2; ++dy) {
          mvprintw(p->y + dy, p->x, "████");
        }
        attroff(COLOR_PAIR(CP_P1));
      } else if (currentTime >= p->endTime) {
        for (int dy = 0; dy < 2; ++dy) {
          mvprintw(p->y + dy, p->x, "    ");
        }

        p->isDone = true;
      }

      if (p->isDone) {
        delete p;
        it = pens.erase(it);
      } else {
        ++it;
      }
    }
  }

  bool isFinished() const { return pens.empty(); }

  void startBlockingTransition(int y0 = 0, int x0 = 0, int y1 = 0, int x1 = 0,
                               int sweepTicks = 12, int jitterMax = 5,
                               int t1min = 40, int t1max = 50) {
    if (y1 == 0 || x1 == 0) {
      int max_y, max_x;
      getmaxyx(stdscr, max_y, max_x);
      fadeLeftToRight(y0, x0, max_y, max_x, sweepTicks, jitterMax, t1min,
                      t1max);
    } else {
      fadeLeftToRight(y0, x0, y1, x1, sweepTicks, jitterMax, t1min, t1max);
    }

    for (int i = 0; i < sweepTicks; ++i) {
      updateAndDraw();
      refresh();
      napms(kFrameMs);
    }
  }
};

/*
int main() {
    setlocale(LC_ALL, "");

    initscr();
    noecho();
    cbreak();
    curs_set(0);
    timeout(0);

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    BlizzardEffect blizzard;

    // Updated parameters:
    // sweepTicks = 20 (faster sweep)
    // jitterMax = 5 (tighter edge)
    // t1min = 60, t1max = 100 (persists longer)
    blizzard.fadeLeftToRight(0, 0, max_y, max_x, 20, 5, 60, 100);

    while (!blizzard.isFinished()) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        }

        blizzard.updateAndDraw();
        refresh();

        napms(33);
    }

    endwin();

    return 0;
}

*/
