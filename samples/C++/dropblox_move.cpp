#include "dropblox_ai.h"

// Asserts: point is legal
// Nothing is between block and location
Board* place_rotate(Board * board, 
                    const int rotation,
                    const int i,
                    const int j) {
  Block *block = board->block;
  for (int k = 0; k < rotation; ++k) {
    block->rotate();
  }
  const int blockJ = block->center.j;
  if (blockJ < j) {
    for (int k = 0; k < j - blockJ; ++k) {
      block->right();
    }
  } else {
    for (int k = 0; k < blockJ - j; ++k) {
      block->left();
    }
  }
  return board->place();
}
