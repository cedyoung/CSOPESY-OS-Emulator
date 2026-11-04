#include "scheduler.h"
#include "process.h"

#include <iostream>
#include <string>
#include <sstream>

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


void openProcessScreen(Process* proc) {
    bool inScreen = true;
    while (inScreen) {
        std::cout << proc->name << "\\> ";
        std::string command;
        std::getline(std::cin, command);

        if (command == "process-smi") {
            std::cout << "\nProcess name: " << proc->name << "\n";
            std::cout << "ID: " << proc->processId << "\n";
            std::cout << "Logs:\n";
            for (const auto& log : proc->logs) {
                std::cout << "(" << log.timestamp << ") Core: " << log.coreId 
                          << " \"" << log.message << "\"\n";
            }
            std::cout << "\nCurrent instruction line: " << proc->currentInstruction << "\n";
            std::cout << "Lines of code: " << proc->totalInstructions << "\n";
            
            if (proc->state == ProcessState::FINISHED) {
                std::cout << "Finished!\n";
            }
            std::cout << "\n";
        }
        else if (command == "exit") {
            inScreen = false;
        }
        else {
            std::cout << "Command not recognized in process screen. Type 'process-smi' or 'exit'.\n";
        }
    }
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
                continue;
            }

            std::stringstream ss(command);
            std::string action;
            ss >> action;

            if (action == "screen") {
                std::string flag;
                ss >> flag;

                if (flag == "-s") {
                    std::string procName;
                    ss >> procName;

                    if (procName.empty()) {
                        std::cout << "Error: Process name cannot be empty. Usage: screen -s <name>\n";
                    } else {
                        scheduler.createProcess(procName);

                        Process* proc = scheduler.findProcess(procName);
                        if (proc != nullptr) {
                            clearScreen();
                            openProcessScreen(proc);
                            clearScreen();
                            displayHeader();
                        }
                    }
                }
                else if (flag == "-r") {
                    std::string procName;
                    ss >> procName;

                    if (procName.empty()) {
                        std::cout << "Error: Process name cannot be empty. Usage: screen -r <name>\n";
                    } else {
                        Process* proc = scheduler.findProcess(procName);
                        
                        if (proc == nullptr || proc->state == ProcessState::FINISHED) {
                            std::cout << "Process " << procName << " not found.\n";
                        } else {
                            clearScreen();
                            openProcessScreen(proc);
                            clearScreen();
                            displayHeader();
                        }
                    }
                }
                else if (flag == "-ls") {
                    std::vector<Process> activeProcs = scheduler.getActiveProcesses();
                    
                    std::cout << "\n--- Active Processes ---\n";
                    if (activeProcs.empty()) {
                        std::cout << "No processes currently running.\n";
                    } else {
                        for (const auto& p : activeProcs) {
                            std::cout << "Name: " << p.name 
                                      << " | ID: " << p.processId 
                                      << " | Instructions: " << p.currentInstruction << "/" << p.totalInstructions 
                                      << " | State: ";
                            
                            if (p.state == ProcessState::READY) std::cout << "READY";
                            else if (p.state == ProcessState::RUNNING) std::cout << "RUNNING";
                            else if (p.state == ProcessState::FINISHED) std::cout << "FINISHED";
                            
                            std::cout << "\n";
                        }
                    }
                    std::cout << "------------------------\n";
                }
                else {
                    std::cout << "Invalid screen flag. Use 'screen -s <name>', 'screen -r <name>', or 'screen -ls'\n";
                }
            }
        }
    }

    return 0;
}
