#include "cli.h"
#include <iostream>
#include <thread>
#include <sstream>

// Constructor for CommandLineInterface, handles command line Arguments
CommandLineInterface::CommandLineInterface(int argc, char **argv) {
    this->print = true;
    this->ms = 500;
    if (argc >= 2 && argc <= 3) {
        if (argc == 2) {
            std::cout << "Open File:" << argv[1] << std::endl;
            std::string filename = std::string(argv[1]);
            this->world = new World(filename);
        } else if (argc == 3) {
            int width = atoi(argv[1]);
            int height = atoi(argv[2]);
            // Creating new World with given Parameters
            std::cout << "\033[2J\033[H" << "Create new World with width " << width << " and height "
                    << height << std::endl;
            this->world = new World(width, height);
        }
        mainMenu();
    } else {
        std::cout << "Unknown Number of Parameters." << std::endl;
        std::cout << std::endl;
        std::cout << "Kindly add the name of a safestate or the height and width "
                "of the playing field"
                << std::endl;
    }
}

// Handling user input for Game control
void CommandLineInterface::mainMenu() {
    bool run = true;
    while (run) {
        std::cout << "\033[2J\033[H" << "Main Menu" << std::endl;
        if(this->print) this->world->print();
        std::cout << std::endl;
        std::cout << "(a)dd cell" << std::endl;
        std::cout << "(d)isplay settings" << std::endl;
        std::cout << "(n)ext Generation" << std::endl;
        std::cout << "(p)lay simulation" << std::endl;
        std::cout << "(p)lay simulation for n generations" << std::endl;
        std::cout << "(q)uit" << std::endl;
        std::string input;
        char objectType;
        int n;

        std::cout << ">> ";
        std::getline(std::cin, input);
        std::istringstream iss;
        if (!input.empty()) {
            objectType = input[0];
            switch (objectType) {
                case 'n':
                    std::cout << "Next Gen:" << std::endl;
                    this->world->evolve(); // Proceed to the next generation of Cells
                    break;
                case 'p':
                    if (input.size() > 2) {
                        std::string numberString = input.substr(2); // Get the substring from index 2 to the end
                        iss.str(numberString); // Set the string stream to the number string
                        if (iss >> n) this->calculate_processing_time(n);
                    } else if (input.size() == 1) {
                        evolveLoop();
                    }
                    break;
                case 'a':
                    addMenu(); // Enter the edit menu
                    break;
                case 'd':
                    displayMenu(); // Enter the edit menu
                    break;
                case 'q':
                    run = false;
                    saveMenu();
                    break;
                default:
                    std::cout << "Kindly enter a valid value" << std::endl;
            }
        }
    }
}

void CommandLineInterface::addMenu() {
    bool run = true;
    while (run) {
        std::cout << "\033[2J\033[H" << "Add Cells" << std::endl;

        if(this->print) this->world->print();
        std::cout << std::endl;

        std::cout << "(b)eacon (x, y)" << std::endl;
        std::cout << "(c)ell (x, y)(p)" << std::endl;
        std::cout << "(d)elete cell (y, x)(p)" << std::endl;
        std::cout << "(g)lider (x,y)" << std::endl;
        std::cout << "(t)oad (x,y)" << std::endl;
        std::cout << "(m)ethuselah (x,y)" << std::endl;
        std::cout << "(r)andom pattern" << std::endl;
        std::cout << "(q)uit (go back to main menu)" << std::endl;

        std::string input;

        std::cout << ">>";
        std::getline(std::cin, input);

        char objectType;
        int x, y;
        x = -1;
        y = -1;

        std::istringstream iss(input);
        iss >> objectType;
        // Don't proceed, if input is missing
        if (iss.peek() != EOF) iss >> x;
        if (iss.peek() != EOF) iss >> y;

        switch (objectType) {
            case 'b':
                if (x != -1 && y != -1) {
                    this->world->add_beacon(y, x);
                }
                break;
            case 'c':
                // Add a single Cell to a specific position

                if (x != -1 && y != -1) {
                    std::cout << "nach koordinaten" << std::endl;
                    this->world->set_cell_state(1, y, x);
                } else if (x != -1) {
                    std::cout << "nach punkt" << std::endl;
                    this->world->set_cell_state(1, x);
                }
                break;
            case 'd':
                // Delete a single Cell at a specific position
                std::cout << "Füge Zelle ein" << std::endl;
                std::cout << "x: " << x << std::endl;
                std::cout << "y: " << y << std::endl;
                if (x != -1 && y != -1) {
                    std::cout << "nach koordinaten" << std::endl;
                    this->world->set_cell_state(0, y, x);
                } else if (x != -1) {
                    std::cout << "nach punkt" << std::endl;
                    this->world->set_cell_state(0, x);
                }
                break;
            case 't':
                if (x != -1 && y != -1) {
                    this->world->add_toad(y, x);
                }
                break;
            case 'g':
                if (x != -1 && y != -1) {
                    this->world->add_glider(y, x);
                }
                break;
            case 'm':
                if (x != -1 && y != -1) {
                    this->world->add_methuselah(y, x);
                }
                break;
            case 'r':
                this->world->randomize();
                break;
            case 'q':
                run = false;
                break;
            default:
                std::cout << "Kindly enter a valid value" << std::endl;
        }
    }
}

// Loop for controlling the auto play loop
void CommandLineInterface::evolveLoop() {
    std::atomic<bool> run(true);
    std::thread autoPlayThread(&CommandLineInterface::autoPlay, this, std::ref(run));

    char input;

    while (run) {
        std::cin >> input;
        if (input == 'q') {
            run = false;
        }
    }
    // Deleting the thread
    autoPlayThread.join();
};

void CommandLineInterface::autoPlay(std::atomic<bool> &run) {
    while (run) {
        std::cout << "\033[2J\033[H" << "Auto Play | Generation: " << this->world->getGeneration() << std::endl;
        this->world->evolve();
        if(this->print) {
            this->world->print();
        }
        std::cout << "(q)uit (go back to main menu)" << std::endl;
        // Delay.
        std::this_thread::sleep_for(std::chrono::milliseconds(this->ms));
    }
}

void CommandLineInterface::displayMenu() {
    int eingabea;
    bool run = true;
    while (run) {
        std::cout << "\033[2J\033[H" << "Main Menu" << std::endl;
        if(this->print) this->world->print();
        std::cout << "current delay: " << this->ms << "\t|\tprint world update: " << this->print << std::endl;
        std::cout << std::endl;
        std::cout << "(d)elay settings (int ms)" << std::endl;
        std::cout << "(p)print world update (y/n)" << std::endl;
        std::cout << "(q)uit" << std::endl;
        //std::cout << eingabea << std::endl;
        //std::cout << std::isdigit(eingabea) << std::endl;
        std::string input;

        std::cout << ">>";
        std::getline(std::cin, input);


        char objectType;
        int msSetting;
        std::string arr;

        char printSetting;
        msSetting = 500;


        std::istringstream iss(input);
        iss >> objectType;
        if (iss.peek() != EOF) iss >> arr;



        switch (objectType) {
            case 'd':
                try {
                    this->ms = std::stoi(arr);
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Not a number" << std::endl;
                } catch (const std::out_of_range& e) {

                }
                break;
            case 'p':

                if (arr == "y") this->print = true;
                if (arr == "n") this->print = false;
                break;
            case 'q':
                run = false;
                break;
            default:
                std::cout << "Kindly enter a valid value" << std::endl;
        }
    }
}

void CommandLineInterface::saveMenu() {
    bool write_success;
    std::string filename;
    bool run = true;
    while (run) {
        std::cout << "\033[2J\033[H" << "Main Menu" << std::endl;
        if(this->print) this->world->print();
        std::cout << "Would you like to save the current gamestate?";
        std::cout << std::endl;
        std::cout << "(s)ave" << std::endl;
        std::cout << "(q)uit" << std::endl;
        std::string input;

        std::cout << ">> ";
        std::cin >> input;

        switch (input.at(0)) {
            case 's':
                write_success = false;

                while (!write_success) {
                    std::cout << "Please enter the the filename to which you would like to save your gamestate" << std::endl
                            << "(Exluding the file extension, it will be automatically saved as a .txt file)" << std::endl
                            << ">> ";
                    std::cin >> filename;
                    try {
                        this->world->save_gamestate(filename);
                        // If no error encountered.
                        write_success = true;
                    } catch (const std::runtime_error& e) {
                        std::cerr << e.what() << std::endl;
                        std::cout << "Please try again.\n\n";
                    }
                }
                run = false;
                break;
            case 'q':
                run = false;
                break;
            default:
                std::cout << "Kindly enter a valid value" << std::endl;
        }
    }
}

long CommandLineInterface::calculate_processing_time(long generations) {
    long generations_done = 0;

    // Start the clock
    auto start = std::chrono::high_resolution_clock::now();
    while (generations_done < generations) {
        std::cout << "\033[2J\033[H" 
                  << "Running the evolution for additional " + std::to_string(generations) + " generations...\n";
        this->world->evolve();
        generations_done++;
        if(this->print) this->world->print(); 
        if(this->world->is_stable()) break; 
        // Delay.
        std::this_thread::sleep_for(std::chrono::milliseconds(this->ms));
    }
    // Stop the clock
    auto stop = std::chrono::high_resolution_clock::now();

    // Calculate the duration
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    std::cout << "Time taken to run the evolution: " << duration.count() << " microseconds" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));

    return duration.count();
}