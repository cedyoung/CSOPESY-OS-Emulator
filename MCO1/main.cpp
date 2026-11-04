#include "scheduler.h"
#include "process.h"

#include <iostream>
#include <string>

void displayHeader() {
    std::cout << R"(
  _____  _____  ____  _____  ______  _______     __
 / ____|/ ____|/ __ \|  __ \|  ____|/ ____\ \   / /
| |    | (___ | |  | | |__) | |__  | (___  \ \_/ /
| |     \___ \| |  | |  ___/|  __|  \___ \  \   /
| |____ ____) | |__| | |    | |____ ____) |  | |
 \_____|_____/ \____/|_|    |______|_____/   |_|

)";

    std::cout << "Hello, Welcome to CSOPESY commandLine!" << std::endl;
    std::cout << "Type 'exit' to quit" << std::endl;
    std::cout << "** IMPORTANT: Type 'initialize' to start system **" << std::endl;
}

void clearScreen() {
    std::cout << "\033[2J\033[1;1H";
}

int main() {
    FCFSScheduler scheduler;
    Config config;

    bool initialized = false;
    bool running = true;

    displayHeader();

    while (running) {
        std::string command;

        std::cout << "root:\\> ";
        std::getline(std::cin, command);

        if (command == "initialize") {

            if (initialized) {
                std::cout
                    << "System is already initialized.\n";
                continue;
            }

            if (!config.loadConfigFile("config.txt")) {
                std::cout << "Failed to initialize due to config file error.\n";
                continue;
            }

            

            scheduler.initialize(config);
            scheduler.start();

            initialized = true;

            std::cout
                << "System initialized successfully.\n";
        }

        else if (command == "exit") {

            if (initialized) {
                scheduler.stop();
            }

            running = false;
        }
        else if (command == "test_process") {
            if (!initialized) {
                std::cout << "Please initialize the system first.\n";
            } else {
                
                Process myProcess("p1", 1, 5);
                std::cout << "Created process: " << myProcess.name << " with 5 instructions.\n";

                
                while (myProcess.state != ProcessState::FINISHED) {
                    bool isFinished = myProcess.executeStep();
                    std::cout << "Executing instruction... Current progress: " 
                              << myProcess.currentInstruction << "/" << myProcess.totalInstructions << "\n";
                    
                    if (isFinished) {
                        std::cout << "Process " << myProcess.name << " has FINISHED!\n";
                    }
                }
            }
        }
        else {

            if (!initialized) {
                std::cout
                    << "Please initialize the system first.\n";
            }
            else {
                std::cout
                    << "Command not implemented yet.\n";
            }
        }
    }

    return 0;
}
