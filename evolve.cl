__kernel void evolve(__global const int* grid,
                     __global int* newGrid,
                     const int width, const int height) {
  int x = get_global_id(0);
  int y = get_global_id(1);

  int determinationValue = calcDeterminationValue(grid, x, y, width, height);

  if (determinationValue == 2) {
    newGrid[y * width + x] = grid[y * width + x];
  } else if (determinationValue == 3) {
    newGrid[y * width + x] = 1;
  } else {
    newGrid[y * width + x] = 0;
  }
}


int calcDeterminationValue(__global int* grid, int x, int y, int width, int height) {
  int determinationValue = 0;

  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (dx == 0 && dy == 0) continue;

      int sx = (x + dx + width) % width;
      int sy = (y + dy + height) % height;

      determinationValue += grid[sy * width + sx];
    }
  }

  return determinationValue;
}
