#include "scheduler.h"

#include <iomanip>
#include <iostream>
#include <sstream>

int main() {
    FCFSScheduler scheduler;

    // Required test case:
    // Create 10 processes with 100 commands each.
    for (int i = 1; i <= 10; ++i) {
        std::ostringstream processName;

        processName
            << "process_"
            << std::setw(2)
            << std::setfill('0')
            << i;

        scheduler.createProcess(
            processName.str(),
            100
        );
    }

    scheduler.start();

    std::cout
        << "CSOPESY FCFS Scheduler - Week 5 Homework\n";

    std::cout
        << "10 processes created, each with 100 print commands.\n";

    std::cout
        << "CPU cores: "
        << FCFSScheduler::CPU_CORES
        << "\n\n";

    std::cout << "Commands:\n";
    std::cout
        << "  screen -ls   Show running and finished processes\n";
    std::cout
        << "  exit         Close the emulator\n";
    std::cout
        << "  help         Show commands\n\n";
    
    std::string command;
    // Scheduler execution will be enabled
    // once the scheduling functions are implemented.
    //

    while (true) {
        std::cout << "Enter a command: ";

        if (!std::getline(std::cin, command)) {
            break;
        }

        if (command == "screen -ls") {
            scheduler.printScreenList();

            if (scheduler.allFinished()) {
                std::cout
                    << "All processes are finished.\n\n";
            }
        }
        else if (command == "exit") {
            if (!scheduler.allFinished()) {
                std::cout
                    << "Warning: some processes are still running.\n";
            }

            break;
        }
        else if (command == "help") {
            std::cout
                << "screen -ls   Show process status\n";
            std::cout
                << "exit         Close the emulator\n";
            std::cout
                << "help         Show commands\n";
        }
        else if (!command.empty()) {
            std::cout
                << "Unknown command. Type 'help'.\n";
        }
    }

    scheduler.stop();
    return 0;
}
