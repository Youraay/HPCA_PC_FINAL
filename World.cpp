#include "World.h"
#include <iostream>
#include <ostream>
#include <stdexcept>

#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <vector>

// Constructor for World with given height or width
World::World(int height, int width) {
  this->height = height;
  this->width = width;
  this->generation = 0;
  this->cell_states.resize(height * width);
  this->grid.resize(height, std::vector<int>(width, 0));
  this->patterns = {'b', 'g', 'm', 't'};
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
  this->cell_states.resize(height * width);
  this->grid.resize(height, std::vector<int>(width, 0));
  this->patterns = {'b', 'g', 'm', 't'};

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
  std::vector<std::vector<int>> newGrid(this->height, std::vector<int>(this->width, 0));

  /* Go through all cells in the current grid and determine whether they:
  1. Die, as if by underpopulation or overpopulation
  2. Continue living on to the next generation
  3. Come to life, as if by reproduction */
  for (int y = 0; y < this->height; y++) {
    for (int x = 0; x < this->width; x++) {
      int determinationValue = calcDeterminationValue(y, x);
      if (determinationValue == 2) {
        newGrid[y][x] = this->grid[y][x];
      } else if (determinationValue == 3) {
        newGrid[y][x] = 1;
      } else {
        newGrid[y][x] = 0;
      }
    }
  }
  // Overwrite old with new grid and increment generation counter
  this->generation += 1;
  this->grid = newGrid;
}

void World::randomize() {
  // Copy the current state of the world
  World world_copy(*this);
  while (are_worlds_identical(grid, world_copy.grid))
  {
    // seed random number generator with current time
  srand(time(0));

  // random starting cell for pattern
  int x = rand() % this->width;
  int y = rand() % this->height;

  // random pattern among the ones implemented
  int patternIndex = rand() % this->patterns.size();

  char pattern = this->patterns[patternIndex];

  switch (pattern)
  {
  case 'b':
    add_beacon(y, x);
    break;
  case 'g':
    add_glider(y, x);
    break;
  case 'm':
    add_methuselah(y, x);
    break;
  case 't':
    add_toad(y, x);
    break;
  
  default:
    break;
  }
  }
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

int World::calcDeterminationValue(int y, int x) {
  // Variable for counting all living neighbors
  int determinationValue = 0;
  // Go through all adjacent cells and add them up
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <=1; dx++) {
      if (dx == 0 && dy ==0) continue;

      int sx = (x+ dx+ this->width) % this->width;
      int sy = (y+ dy+ this->height) % this->height;

      determinationValue += this->grid[sy][sx];
    }
  }
  // Return the number of living neighbors of a given cell.
  // (e.g. 2 neighbors = 7*0 + 2*1)
  return determinationValue;
}

bool World::are_worlds_identical(const std::vector<std::vector<int>>& grid_1,
                                 const std::vector<std::vector<int>>& grid_2) {
  // Compare each cell in both worlds
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      // return false, if any cell is different
      if (grid_1[i][j] != grid_2[i][j]) {
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
    return grid[y][x] ? 1 : 0;
  }
  // Return -1 and print error message, if coordinates are invalid
  else {
    std::cerr << "Invalid coordinates: (" << x << ", " << y << ")" << std::endl;
    return -1;
  }
}

int World:: get_cell_state(int p) {
  // Determine 2d coordinates from point and return cell state, if point is valid
  if ((this->height * this-> width) >= p) {
    int y = p / this->width;
    int x = p % this->width;
    return grid[y][x] ? 1 : 0;
  }
  // Return -1 and print error message, if if point is invalid
  else {
    std::cerr << "Point" << p << "is out of range." << std::endl;
    return -1;
  }
}

void World::set_cell_state(int state, int y, int x) {
  // Set the cell state, if coordinates are valid
  if (x >= 0 && x < width && y >= 0 && y < height) {
    this->grid[y][x] = state;
  }
  // Print error message, if coordinates are invalid
  else {
    std::cerr << "Invalid coordinates: (" << x << ", " << y << ")" << std::endl;
  }
};

void World::set_cell_state(int state, int p) {
  bool succ = true; // Who is bro succing 🤨?
  int cell_count = this->height * this->width;
  // Determine 2d coordinates from point and set cell state, if point is valid
  if (cell_count >= p) {
    int y = p / this->width;
    int x = p % this->width;
    this->grid[y][x] = state;
    // Print error message, if point is invalid
  } else {
    std::cerr << "Point" << p << "is out of range." << std::endl;
  }
}

void World::add_beacon(int y, int x) {
  // Add beacon pattern, if coordinates are in bounds
  if (x >= 0 && y >= 0 && x+3 < width && y+3 < height) {
    this->grid[y][x] = 1;
    this->grid[y+1][x] = 1;
    this->grid[y][x+1] = 1;

    this->grid[y+3][x+3] = 1;
    this->grid[y+3][x+2] = 1;
    this->grid[y+2][x+3] = 1;
  }
}

void World::add_glider(int y, int x) {
  // Add glider pattern, if coordinates are in bounds
  if (x >= 0 && y >= 0 && x+3 < width && y+3 < height) {
    this->grid[y][x+1] = 1;
    this->grid[y+1][x+2] = 1;

    this->grid[y+2][x] = 1;
    this->grid[y+2][x+1] = 1;
    this->grid[y+2][x+2] = 1;
  }
}

void World::add_methuselah(int y, int x) {
  // Add methuselah pattern, if coordinates are in bounds
  if (x >= 0 && y >= 0 && x+2 < width && y+2 < height) {
    this->grid[y][x+1] = 1;
    this->grid[y][x+2] = 1;
    this->grid[y+1][x] = 1;
    this->grid[y+1][x+1] = 1;
    this->grid[y+2][x+1] = 1;
  }
}

void World::add_toad(int y, int x) {
  // Add toad pattern, if coordinates are in bounds
  if (x >= 0 && y >= 0 && x+3 < width && y+3 < height) {
    this->grid[y][x+2] = 1;
    this->grid[y+1][x] = 1;
    this->grid[y+2][x] = 1;
    this->grid[y+1][x+3] = 1;
    this->grid[y+2][x+3] = 1;
    this->grid[y+3][x+1] = 1;
  }
}

void World::print() {
  for (auto & i : grid) {
    for (int j : i) {
      if (j) {
        std::cout << "\033[32mx\033[0m" << " ";
      } else {
        std::cout << "\033[90mo\033[0m" << " ";
      }

    }
    std::cout << std::endl;
  }
}

long World::getGeneration() {
  return this-> generation;
}

