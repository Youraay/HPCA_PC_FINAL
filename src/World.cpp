#include "World.h"

#include "../Vc/Vc/Vc"

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
  this->N = this->height * this->width;
  this->generation = 0;
  this->grid = new int[height * width];
  std::fill_n(this->grid, this->height * this->width, 0);
  this->cl = NULL;
  this->patterns = {'b', 'g', 'm', 't'};
  std::cout << "WORLD CREATED." << std::endl;
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
            iss >> this->height;
          } else if (key == "width") {
            iss >> this->width;
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

  this->N = this->height * this->width;
  this->generation = 0;
  this->grid = new int[this->height * this->width];
  std::fill_n(this->grid, this->height * this->width, 0);
  this->cl = NULL;
  this->patterns = {'b', 'g', 'm', 't'};

  // Set cell states.
  for (size_t i = 0; i < startPositions.size(); i++) {
    std::pair<int, int> pos = startPositions.at(i);
    this->set_cell_state(true, pos.second, pos.first);
  }

  std::cout << "WORLD CREATED." << std::endl;
}

World::~World() {
  delete this->cl;
  delete[] this->grid; 
}

void World::init_OpenCL() {
  delete this->cl;
  this->cl = new OpenCLWrapper(*this);
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

int* World::evolve() {
  /*
  * Go through all cells in the current grid and determine whether they:
  *  1. Die, as if by underpopulation or overpopulation
  *  2. Continue living on to the next generation
  *  3. Come to life, as if by reproduction 
  */
  // Declare new grid
  int* newGrid = new int[this->N];

  // Write current grid to buffer only if it has changed
  cl->err = clEnqueueWriteBuffer(cl->queue, cl->buffer_grid, CL_TRUE, 0, sizeof(int) * this->N, grid, 0, NULL, NULL);
  cl->checkError(cl->err, "clEnqueueWriteBuffer");


  // Run the kernel (evolve) function using the GPU.
  cl->err = clEnqueueNDRangeKernel(cl->queue, cl->kernel_evolve, 2, NULL, cl->evolve_global_work_size, NULL, 0, NULL, NULL);
  cl->checkError(cl->err, "clEnqueueNDRangeKernel");
  // Read the resulting new grid from the buffer into host memory (newGrid).
  cl->err = clEnqueueReadBuffer(cl->queue, cl->buffer_newGrid, CL_TRUE, 0, sizeof(int) * this->N, newGrid, 0, NULL, NULL);
  cl->checkError(cl->err, "clEnqueueReadBuffer");

  // Overwrite old with new grid and increment generation counter.
  if (memory_safety) delete[] this->grid;
  this->grid = newGrid;
  this->generation++;

  return this->grid;
}

void World::randomize() {
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

// OpenCL VERSION
/* bool World::are_worlds_identical(int* grid_1, int* grid_2) {
  int host_result = CL_TRUE;

  // Create new buffer for grid1.
  cl->err = clEnqueueWriteBuffer(cl->queue, cl->buffer_grid1, CL_TRUE, 0, sizeof(int) * this->N, grid_1, 0, NULL, NULL);
  cl->checkError(cl->err, "clEnqueueWriteBuffer (buffer_grid1)");
  // Create new buffer for grid2.
  cl->err = clEnqueueWriteBuffer(cl->queue, cl->buffer_grid2, CL_TRUE, 0, sizeof(int) * this->N, grid_2, 0, NULL, NULL);
  cl->checkError(cl->err, "clEnqueueWriteBuffer (buffer_grid2)");
  // Write result to buffer only if it has changed
  cl->err = clEnqueueWriteBuffer(cl->queue, cl->buffer_result, CL_TRUE, 0, sizeof(int), &host_result, 0, NULL, NULL);
  cl->checkError(cl->err, "clEnqueueWriteBuffer (buffer_result)");

  // Run the kernel (compare_arrays) function using the GPU.
  cl->err = clEnqueueNDRangeKernel(cl->queue, cl->kernel_compare, 1, NULL, cl->compare_global_work_size, NULL, 0, NULL, NULL);
  cl->checkError(cl->err, "clEnqueueNDRangeKernel");
  // Read the result (whether they are equal or not) from the buffer into host memory (host_result).
  cl->err = clEnqueueReadBuffer(cl->queue, cl->buffer_result, CL_TRUE, 0, sizeof(int), &host_result, 0, NULL, NULL);
  cl->checkError(cl->err, "clEnqueueReadBuffer");

  return (bool)host_result;
} */


// OpenMP + Vc VERSION
bool World::are_worlds_identical(int* grid_1, int* grid_2) {
  // Create a flag to indicate if the worlds are identical
  bool identical = true;
  // Some helping varibales
  const int grid_size = height * width;
  const int vec_size = Vc::int_v::Size;
  const int par_loop_end = grid_size - (grid_size % vec_size);

  // As vector size is 4, handle only a multiple of 4 many elements.
  #pragma omp parallel for shared(identical)
  for (int i = 0; i < par_loop_end; i += vec_size) {
    Vc::int_v vec1(&grid_1[i], Vc::Aligned);
    Vc::int_v vec2(&grid_2[i], Vc::Aligned);

    if (!Vc::all_of(vec1 == vec2)) {
      #pragma omp atomic write
      identical = false;
    }
  }

  // Handle remaining elements seuqentially
  for (int i = par_loop_end; i < grid_size; ++i) {
    if (grid_1[i] != grid_2[i]) {
      identical = false;
      break;
    }
  }

  return identical;
}

// SCALAR VERSION
/*
bool World::are_worlds_identical(int* grid_1, int* grid_2) {
  // Compare each cell in both worlds
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      // return false, if any cell is different
      if (grid_1[j * width + i] != grid_2[j * width + i]) {
        return false;
      }
    }
  }
  // return true, if all the cells are the same
  return true;
}
*/

int World::get_cell_state(int y, int x) {
  // Return cell state, if coordinates are valid
  if (x >= 0 && x < width && y >= 0 && y < height) {
    return grid[y * width + x];
  }
  // Return -1 and print error message, if coordinates are invalid
  else {
    std::cerr << "Invalid coordinates: (" << x << ", " << y << ")" << std::endl;
    return -1;
  }
}

int World::get_cell_state(int p) {
  // Determine 2d coordinates from point and return cell state, if point is valid
  if (this->N >= p) {
    return grid[p];
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
  if (cell_count >= p) {
    this->grid[p] = state;
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




