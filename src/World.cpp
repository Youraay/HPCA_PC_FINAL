#include "World.h"

#include <iostream>
#include <ostream>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>

/*
* To compile your source code, please use the following command to link the OpenCL library: 
* g++ MatMul.cpp -o MatMul -O3 -fopenmp -fno-tree-vectorize -I/usr/include/gegl-0.4 -I~/Vc/include -L~/Vc/lib ~/Vc/lib/libVc.a -L/usr/lib64 /usr/lib64/libOpenCL.so.1
*/

// Constructor for World with given height or width
World::World(int height, int width) {
  this->height = height;
  this->width = width;
  this->generation = 0;
  this->grid = new int[height * width];
  this->cl = OpenCLWrapper(*this);
}

/* Constructor for World with given file, including height, width
and living cell distribution */
World::World(std::string &file_name) {
  std::vector<std::pair<int, int> > startPositions;

  file_name = "configurations/" + file_name;
  if (!std::filesystem::exists(file_name)) {
    throw std::runtime_error("File \"" + file_name + "\" doesn't exist.");
  } else {
    std::ifstream file(file_name);
    if (file.is_open()) {
      std::string line;
      while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        char equalsign;
        if (iss >> key >> equalsign) {
          // Parse height and width.
          if (key == "height") {
            iss >> height;
          } else if (key == "width") {
            iss >> width;
          } else if (key == "start") {
            // Parse start positions
            char nan;
            std::pair<int, int> pos;
            while (iss >> nan >> pos.first >> nan >> pos.second >> nan) {
              startPositions.push_back(pos);
              /* Consume the comma that seperates the pairs.
              Leading whitespaces are ignored. */
              iss >> nan;
            }
          }
        }
      }
      file.close();
    } else {
      std::cerr << "Unable to open file: " << file_name << std::endl;
    }
  }

  this->height = height;
  this->width = width;
  this->generation = 0;
  this->grid = new int[height * width];
  this->cl = OpenCLWrapper(*this);

  // Set cell states.
  for (int i = 0; i < startPositions.size(); i++) {
    std::pair<int, int> pos = startPositions.at(i);
    this->set_cell_state(true, pos.second, pos.first);
  }


}


void World::save_gamestate(std::string file_name) {
  file_name = "configurations/" + file_name + ".txt";
  // Check if file already exists.
  if (std::filesystem::exists(file_name)) {
    throw std::runtime_error("File already exists: " + file_name);
  }
  // If not then proceed with creating it.
  std::ofstream file(file_name);
  if (file.is_open()) {
      std::string text = "height = " + std::to_string(this->height);
      text += "\nwidth = " + std::to_string(this->width);
      text += "\nstart =";
      for (int x = 0; x < this->width; x++) {
        for (int y = 0; y < this->height; y++) {
          if (this->get_cell_state(y, x)) {
            text += " (" + std::to_string(y) + "," + std::to_string(x) + "),";
          }
        }
      }
    file << text;
    file.close();
  } else {
    throw std::runtime_error("Unable to create file: " + file_name);
  } 
}

void World::evolve() {
  // Declare new grid
  int* newGrid = new int[this->height * this->width];

  /* Go through all cells in the current grid and determine whether they:
  1. Die, as if by underpopulation or overpopulation
  2. Continue living on to the next generation
  3. Come to life, as if by reproduction */
  cl.err = clEnqueueNDRangeKernel(cl.queue, cl.kernel, 2, NULL, cl.global_work_size, NULL, 0, NULL, NULL);
  cl.checkError(cl.err, "clEnqueueNDRangeKernel");

  cl.err = clEnqueueReadBuffer(cl.queue, cl.buffer_newGrid, CL_TRUE, 0, sizeof(int) * this->height * this->width, newGrid, 0, NULL, NULL);
  cl.checkError(cl.err, "clEnqueueReadBuffer");

  // Overwrite old with new grid and increment generation counter
  this->generation += 1;
  this->grid = newGrid;
}

bool World::is_stable() {
  // Copy the current state of the world
  World world_copy(*this);
  // evolve the copy to the next generation
  world_copy.evolve();
  // If there is a difference, check if there are oscillators of period 2.
  if (!are_worlds_identical(grid, world_copy.grid)) {
    // evolve the copy again
    world_copy.evolve();
    /* If there is a difference, there are oscillators of periods higher than 2
    or spaceships a.k.a. the world is unstable*/
    if (!are_worlds_identical(grid, world_copy.grid)) {
      return false;
    }
  }
  // if all cells are either the same, or oscillators of period 2, the world is stable
  return true;
}

bool World::are_worlds_identical(int* grid_1, int* grid_2) {
  // Compare each cell in both worlds
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      // return false, if any cell is different
      if (grid_1[i * width + j] != grid_2[i * width + j]) {
        return false;
      }
    }
  }
  // return true, if all the cells are the same
  return true;
}

int World::get_cell_state(int y, int x) {
  // Return cell state, if coordinates are valid
  if (x >= 0 && x < width && y >= 0 && y < height) {
    return grid[y * width + x] ? 1 : 0;
  }
  // Return -1 and print error message, if coordinates are invalid
  else {
    std::cerr << "Invalid coordinates: (" << x << ", " << y << ")" << std::endl;
    return -1;
  }
}

int World::get_cell_state(int p) {
  // Determine 2d coordinates from point and return cell state, if point is valid
  if ((this->height * this->width) >= p) {
    int y = p / this->width;
    int x = p % this->width;
    return grid[y * width + x] ? 1 : 0;
  }
  // Return -1 and print error message, if point is invalid
  else {
    std::cerr << "Point " << p << " is out of range." << std::endl;
    return -1;
  }
}

void World::set_cell_state(int state, int y, int x) {
  // Set the cell state, if coordinates are valid
  if (x >= 0 && x < width && y >= 0 && y < height) {
    this->grid[y * width + x] = state;
  }
  // Print error message, if coordinates are invalid
  else {
    std::cerr << "Invalid coordinates: (" << x << ", " << y << ")" << std::endl;
  }
}

void World::set_cell_state(int state, int p) {
  int cell_count = this->height * this->width;
  // Determine 2d coordinates from point and set cell state, if point is valid
  if (cell_count >= p) {
    int y = p / this->width;
    int x = p % this->width;
    this->grid[y * width + x] = state;
  }
  // Print error message, if point is invalid
  else {
    std::cerr << "Point " << p << " is out of range." << std::endl;
  }
}

// The rest of the methods can be modified in a similar way.


void World::add_beacon(int y, int x) {
  // Add beacon pattern, if coordinates are in bounds
  if (x >= 0 && y >= 0 && x+3 < width && y+3 < height) {
    this->grid[y * width + x] = 1;
    this->grid[(y+1) * width + x] = 1;
    this->grid[y * width + x+1] = 1;

    this->grid[(y+3) * width + x+3] = 1;
    this->grid[(y+3) * width + x+2] = 1;
    this->grid[(y+2) * width + x+3] = 1;
  }
  // Print error message if coordinates are out of bounds
  else {
    std::cerr << "Invalid coordinates to spawn beacon in." << std::endl;
  }
}

void World::add_glider(int y, int x) {
  // Add glider pattern, if coordinates are in bounds
  if (x >= 0 && y >= 0 && x+3 < width && y+3 < height) {
    this->grid[y * width + x+1] = 1;
    this->grid[(y+1) * width + x+2] = 1;

    this->grid[(y+2) * width + x] = 1;
    this->grid[(y+2) * width + x+1] = 1;
    this->grid[(y+2) * width + x+2] = 1;
  }
  // Print error message if coordinates are out of bounds
  else {
    std::cerr << "Invalid coordinates to spawn glider in." << std::endl;
  }
}

void World::add_methuselah(int y, int x) {
  // Add methuselah pattern, if coordinates are in bounds
  if (x >= 0 && y >= 0 && x+2 < width && y+2 < height) {
    this->grid[y * width + x+1] = 1;
    this->grid[y * width + x+2] = 1;
    this->grid[(y+1) * width + x] = 1;
    this->grid[(y+1) * width + x+1] = 1;
    this->grid[(y+2) * width + x+1] = 1;
  }
  // Print error message if coordinates are out of bounds
  else {
    std::cerr << "Invalid coordinates to spawn methuselah in." << std::endl;
  }
}

void World::add_toad(int y, int x) {
  // Add toad pattern, if coordinates are in bounds
  if (x >= 0 && y >= 0 && x+3 < width && y+3 < height) {
    this->grid[y * width + x+2] = 1;
    this->grid[(y+1) * width + x] = 1;
    this->grid[(y+2) * width + x] = 1;
    this->grid[(y+1) * width + x+3] = 1;
    this->grid[(y+2) * width + x+3] = 1;
    this->grid[(y+3) * width + x+1] = 1;
  }
  // Print error message if coordinates are out of bounds
  else {
    std::cerr << "Invalid coordinates to spawn toad in." << std::endl;
  }
}

void World::print() {
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      if (grid[i * width + j]) {
        std::cout << "\033[32mx\033[0m" << " ";
      } else {
        std::cout << "\033[90mo\033[0m" << " ";
      }
    }
    std::cout << std::endl;
  }
}

long World::getGeneration() {
  return this->generation;
}



