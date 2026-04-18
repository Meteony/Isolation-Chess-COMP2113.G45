#pragma once

struct Coord {
  int row = 0;
  int col = 0;

  bool operator==(const Coord& other) const {
    return row == other.row && col == other.col;
  }

  bool operator!=(const Coord& other) const { return !(*this == other); }
};
