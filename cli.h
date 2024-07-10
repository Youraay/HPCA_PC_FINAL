#include <string>
#include <vector>
#include <atomic>

class CommandLineInterface {
private:
    World* world{};
    bool print;
    int ms;
public:
    CommandLineInterface(int argc, char** argv);
    //~CommandLineInterface();
    void mainMenu();
    void addMenu();
    void autoPlay(std::atomic<bool>& run);
    void evolveLoop();
    void displayMenu();
    void saveMenu();

    /**
     * @brief  Run the evolution for a given n number of generations and return the time required. 
     * Break the simulation if a stable state is reached.
     * 
     * @param generations The amount of additional generations to run.
     * 
     * @returns The time required to run the number of generations in milliseconds.
    */
    long calculate_processing_time(long generations);

    //  bool create_world(int hight, int widht); <= Das wird bereits getan durch die Argumente.
    //  bool load_gamestate(std::string name); <= Das wird ebenfalls bereits durch die Argumente getan.
    //  bool save_gamestate();  <= wurde in der World Klasse gemacht.
    //  bool show_progress();
    //  bool hide_progress();
    //  bool set_visualisation_delay(); <= Wird das nicht bereits durch this->ms gemacht?
};
