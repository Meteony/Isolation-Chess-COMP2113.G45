#pragma once

struct Coord {
  int row = 0;
  int col = 0;

  // Returns true if this coordinate matches other.
  bool operator==(const Coord& other) const {
    return row == other.row && col == other.col;
  }

  // Returns true if this coordinate differs from other.
  bool operator!=(const Coord& other) const { return !(*this == other); }
};
