#ifndef WORLD_H
#define WORLD_H

#include "OpenCLWrapper.h"
#include <vector>

class World {
private:
    int height; // Height in cells.
    int width; // Width in cells.
    ulong N; // Total number of cells (Height * Width).
    long int generation; // Generation of the Game of Life.
    bool* grid; // 2-dimensional Grid of Cells, Alive = 1, Dead = 0
    OpenCLWrapper* cl;
    std::vector<char> patterns; // list of patterns, instertable into the world
    bool memory_safety = true;

    friend class CommandLineInterface;
    friend class OpenCLWrapper;

    /**
     * @brief Calculates a new generation of the world/grid to simulate an evolution of the cells.
     * The rules are from the wikipedia article.
     * 
     * @returns The grid of the world after the evolution.
    */
    bool* evolve();

    /**
     * @brief Create a random pattern in a random location (cell) of the world.
     * The starting position i.e. the chosen cell will be the bottom left corner of the generated cell.
     */
    void randomize();

public:
    /**
     * @brief Construct a new World object given height and width.
     *
     * @param height The height in cells/pixels
     * @param width  The width in cells/pixels.
     */
    World(int height, int width);

    /**
     * @brief Construct a new World object given file
     * (that includes height, width and a start distribution of living cells)
     *
     * The file should be in the configurations folder.
     *
     * @param file_name The name of the configuration file in the configurations
     * folder.
     */
    World(std::string &file_name);

    /**
     * @brief Destructor of the World object.
     */
    ~World();

    /**
     * @brief Initializes OpenCL using the OpenCLWrapper constructor and storing it in this->cl;
     */
    void init_OpenCL();

    /**
     * @brief  Save the current world to a file (dimensions and cell states).
     * 
     * @param file_name The file name (excluding extension) to which to save the current world.
    */
    void save_gamestate(std::string file_name);

    /**
     * @brief Calculates the number of neighbors of a given cell.
     * 
     * @param x The x coordinate of the cell.
     * @param y The y coordinate of the cell.
     * 
     * @return The number of living neighbors of a given cell.
    */
    int calcDeterminationValue(int y, int x);

    /**
     *  @brief Checks whether two worlds are identical
     * 
     *  @param grid_1 The first world.
     *  @param grid_2 The second world to compare.
     *  
     *  @return True or false, depending on whether they are identical.
    */
    bool are_worlds_identical(bool* grid_1, bool* grid_2);

    /**
     * @brief  Get a cell state given a two-dimensional grid position (x, y).
     * 
     * @param x The x coordinate.
     * @param y The y coordinate.
     * 
     * @return 1 if alive, 0 if dead.
    */
    int get_cell_state(int y, int x);

    /**
     * @brief Get a cell state given a one-dimensional grid position (p).
     * Uses modulo and division to calculate the x and y coordinates.
     * 
     * @param p The one-dimensional coordinate.
     * 
     * @return 1 if alive, 0 if dead
    */
    int get_cell_state(int p);

    /**
     * @brief Set a cell state (1 = alive, 0 = dead) given a two-dimensional grid position (x, y) and the desired cell state.
     * 
     * @param state The new state of the cell.
     * @param x The x coordinate of the cell.
     * @param y The y coordinate of the cell.
    */
    void set_cell_state(int state, int y, int x);

    /**
     * @brief Set a cell state (1 = alive, 0 = dead) given a one-dimensional grid position (p) and the desired cell state.
     * 
     * @param state The new state of the cell.
     * @param p The one-dimensional coordinate of the cell.
    */
    void set_cell_state(int state, int p);

    /**
     * @brief Add a "Glider" at a given two-dimensional grid position (x, y).
     * 
     * @param x The x coordinate of the starting position (see PDF).
     * @param y The y coordinate of the starting position (see PDF).
    */
    void add_glider(int y, int x);

    /**
     * @brief Add a "Toad" at a given two-dimensional grid position (x, y).
     * 
     * @param x The x coordinate of the starting position (see PDF).
     * @param y The y coordinate of the starting position (see PDF).
    */
    void add_toad(int y, int x);

    /**
     * @brief Add a "Beacon" at a given two-dimensional grid position (x, y).
     * 
     * @param x The x coordinate of the starting position (see PDF).
     * @param y The y coordinate of the starting position (see PDF).
    */
    void add_beacon(int y, int x);

    /**
     * @brief Add a "ethuselah" at a given two-dimensional grid position (x, y).
     * 
     * @param x The x coordinate of the starting position (see PDF).
     * @param y The y coordinate of the starting position (see PDF).
    */
    void add_methuselah(int y, int x);

    /**
     * @brief Prints the world/grid into the console.
    */
    void print();

    /**
     * @brief Getter function of the generation of the world.
     * 
     * @return The generation of the world (long int).
    */
    long getGeneration();
};

#endif // WORLD_H
