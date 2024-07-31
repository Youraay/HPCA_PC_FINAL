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
* g++ MatMul.cpp -o MatMul -O3 -fno-tree-vectorize -I/usr/include/gegl-0.4 -L/usr/lib64 /usr/lib64/libOpenCL.so.1
*/

// Constructor for World with given height or width
World::World(int height, int width) {
  this->height = height;
  this->width = width;
  this->N = this->height * this->width;
  this->generation = 0;
  this->grid = new bool[this->N];
  std::fill_n(this->grid, this->N, 0);
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
  this->grid = new bool[this->N];
  std::fill_n(this->grid, this->N, 0);
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
            text += " (" + std::to_string(x) + "," + std::to_string(y) + "),";
          }
        }
      }
    file << text;
    file.close();
  } else {
    throw std::runtime_error("Unable to create file: " + file_name);
  } 
}

// OpenCL VERSION
bool* World::evolve() {
  // Go through all cells in the current grid and determine whether they:
  // 1. Die, as if by underpopulation or overpopulation
  // 2. Continue living on to the next generation
  // 3. Come to life, as if by reproduction 
  
  // Declare new grid
  bool* newGrid = new bool[this->N];

  cl_event write_event, kernel_event, read_event;
  cl_ulong time_start, time_end;
  double write_duration, kernel_duration, read_duration;

  // Write current grid to buffer only if it has changed
  cl->err = clEnqueueWriteBuffer(cl->queue, cl->buffer_grid, CL_TRUE, 0, sizeof(bool) * this->N, grid, 0, NULL, &write_event);
  cl->checkError(cl->err, "clEnqueueWriteBuffer");


  // Run the kernel (evolve) function using the GPU.
  cl->err = clEnqueueNDRangeKernel(cl->queue, cl->kernel_evolve, 2, NULL, cl->evolve_global_work_size, NULL, 0, NULL, &kernel_event);
  cl->checkError(cl->err, "clEnqueueNDRangeKernel");
  // Read the resulting new grid from the buffer into host memory (newGrid).
  cl->err = clEnqueueReadBuffer(cl->queue, cl->buffer_newGrid, CL_TRUE, 0, sizeof(bool) * this->N, newGrid, 0, NULL, &read_event);
  cl->checkError(cl->err, "clEnqueueReadBuffer");

  // Overwrite old with new grid and increment generation counter.
  if (memory_safety) delete[] this->grid;
  this->grid = newGrid;
  this->generation++;

  // Profiling information (SEE OpenCLWrapper.cpp:25 BEFORE UNCOMMENTING)
  /*
  clGetEventProfilingInfo(write_event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
  clGetEventProfilingInfo(write_event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
  write_duration = (double)(time_end - time_start) / 1000000.0;

  clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
  clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
  kernel_duration = (double)(time_end - time_start) / 1000000.0;

  clGetEventProfilingInfo(read_event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
  clGetEventProfilingInfo(read_event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
  read_duration = (double)(time_end - time_start) / 1000000.0;

  std::cout << "Profiling Information for evolve:\n";
  std::cout << "Write Buffer Duration: " << write_duration << " ms\n";
  std::cout << "Kernel Execution Duration: " << kernel_duration << " ms\n";
  std::cout << "Read Buffer Duration: " << read_duration << " ms\n";
  */

  return this->grid;
}


// SCALAR VERSION
/*
bool* World::evolve() {
  // Declare new grid
  bool* newGrid = new bool[this->N];

  // Go through all cells in the current grid and determine whether they:
  // 1. Die, as if by underpopulation or overpopulation
  // 2. Continue living on to the next generation
  // 3. Come to life, as if by reproduction
  for (int y = 0; y < this->height; y++) {
    for (int x = 0; x < this->width; x++) {
      // Variable for counting all living neighbors
      int determinationValue = 0;
      // Go through all adjacent cells and add them up
      for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <=1; dx++) {
          if (dx == 0 && dy ==0) continue;

          int sx = (x+ dx+ this->width) % this->width;
          int sy = (y+ dy+ this->height) % this->height;

          determinationValue += this->grid[sy * this->width + sx];
        }
      }
      if (determinationValue == 2) {
        newGrid[y * this->width + x] = this->grid[y * this->width + x];
      } else if (determinationValue == 3) {
        newGrid[y * this->width + x] = 1;
      } else {
        newGrid[y * this->width + x] = 0;
      }
    }
  }

  // Overwrite old with new grid and increment generation counter.
  if (memory_safety) delete[] this->grid;
  this->grid = newGrid;
  this->generation++;

  return this->grid;
}
*/


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
bool World::are_worlds_identical(bool* grid_1, bool* grid_2) {
  int host_result = CL_TRUE;

  cl_event write_event_1, write_event_2, write_event_result, kernel_event, read_event;
  cl_ulong time_start, time_end;
  double write_duration_1, write_duration_2, write_duration_result, kernel_duration, read_duration;

  // Write grid_1 to buffer grid1
  cl->err = clEnqueueWriteBuffer(cl->queue, cl->buffer_grid1, CL_TRUE, 0, sizeof(bool) * this->N, grid_1, 0, NULL, &write_event_1);
  cl->checkError(cl->err, "clEnqueueWriteBuffer (buffer_grid1)");

  // Write grid_2 to buffer grid2
  cl->err = clEnqueueWriteBuffer(cl->queue, cl->buffer_grid2, CL_TRUE, 0, sizeof(bool) * this->N, grid_2, 0, NULL, &write_event_2);
  cl->checkError(cl->err, "clEnqueueWriteBuffer (buffer_grid2)");

  // Write result to buffer result
  cl->err = clEnqueueWriteBuffer(cl->queue, cl->buffer_result, CL_TRUE, 0, sizeof(int), &host_result, 0, NULL, &write_event_result);
  cl->checkError(cl->err, "clEnqueueWriteBuffer (buffer_result)");

  // Run the kernel (compare_arrays) function using the GPU
  cl->err = clEnqueueNDRangeKernel(cl->queue, cl->kernel_compare, 1, NULL, cl->compare_global_work_size, NULL, 0, NULL, &kernel_event);
  cl->checkError(cl->err, "clEnqueueNDRangeKernel");

  // Read the result from buffer result into host memory (host_result)
  cl->err = clEnqueueReadBuffer(cl->queue, cl->buffer_result, CL_TRUE, 0, sizeof(int), &host_result, 0, NULL, &read_event);
  cl->checkError(cl->err, "clEnqueueReadBuffer");

  // Profiling information (SEE OpenCLWrapper.cpp:25 BEFORE UNCOMMENTING)
  /*
  clGetEventProfilingInfo(write_event_1, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
  clGetEventProfilingInfo(write_event_1, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
  write_duration_1 = (double)(time_end - time_start) / 1000000.0;

  clGetEventProfilingInfo(write_event_2, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
  clGetEventProfilingInfo(write_event_2, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
  write_duration_2 = (double)(time_end - time_start) / 1000000.0;

  clGetEventProfilingInfo(write_event_result, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
  clGetEventProfilingInfo(write_event_result, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
  write_duration_result = (double)(time_end - time_start) / 1000000.0;

  clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
  clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
  kernel_duration = (double)(time_end - time_start) / 1000000.0;

  clGetEventProfilingInfo(read_event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
  clGetEventProfilingInfo(read_event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
  read_duration = (double)(time_end - time_start) / 1000000.0;

  std::cout << "Profiling Information for are_worlds_identical:\n";
  std::cout << "Write Buffer 1 Duration: " << write_duration_1 << " ms\n";
  std::cout << "Write Buffer 2 Duration: " << write_duration_2 << " ms\n";
  std::cout << "Write Buffer Result Duration: " << write_duration_result << " ms\n";
  std::cout << "Kernel Execution Duration: " << kernel_duration << " ms\n";
  std::cout << "Read Buffer Duration: " << read_duration << " ms\n";
  */

  return (bool)host_result;
}



// SCALAR VERSION
/*
bool World::are_worlds_identical(bool* grid_1, bool* grid_2) {
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




