__kernel void evolve(const __global bool* grid,
                     __global bool* newGrid,
                     int width, int height) {
  int x = get_global_id(0);
  int y = get_global_id(1);

  int determinationValue = 0;

  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (dx == 0 && dy == 0) continue;

      int sx = (x + dx + width) % width;
      int sy = (y + dy + height) % height;

      determinationValue += grid[sy * width + sx];
    }
  }

  if (determinationValue == 2) {
    newGrid[y * width + x] = grid[y * width + x];
  } else if (determinationValue == 3) {
    newGrid[y * width + x] = 1;
  } else {
    newGrid[y * width + x] = 0;
  }
}


__kernel void compare_arrays(const __global bool* array1,
                             const __global bool* array2,
                             __global int* result, ulong size) {
  if (result) {
    int index = get_global_id(0);
    if (index < size) {
        if (array1[index] != array2[index]) {
            *result = false;
        }
    }
  }
}