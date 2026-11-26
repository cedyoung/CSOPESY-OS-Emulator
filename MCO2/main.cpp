#include "scheduler.h"
#include "process.h"

#include <iomanip>
#include <iostream>
#include <sstream>
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

void clearScreen() { std::cout << "\033[2J\033[1;1H"; }

// --- memory-size validation per spec: power of 2 in [64, 65536] ----------
static bool isValidMemorySize(std::uint64_t v) {
    return v >= 64 && v <= 65536 && (v & (v - 1)) == 0;
}

static bool parseU64(const std::string& s, std::uint64_t& out) {
    if (s.empty()) return false;
    try { std::size_t pos = 0; out = std::stoull(s, &pos); return pos == s.size(); }
    catch (...) { return false; }
}

// --- process screen ------------------------------------------------------
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
            std::cout << "\nCurrent instruction line: "
                      << proc->currentInstruction << "\n";
            std::cout << "Lines of code: " << proc->totalInstructions << "\n";
            std::cout << "Memory: " << proc->memoryRequired << " B ("
                      << proc->numPages << " pages), "
                      << "resident=" << proc->residentPages << ", "
                      << "faults=" << proc->pageFaults << "\n";

            if (proc->state == ProcessState::FINISHED)
                std::cout << "Finished!\n";
            else if (proc->state == ProcessState::VIOLATED) {
                std::cout << "Shut down due to memory access violation error "
                             "that occurred at " << proc->violationTime
                          << ". 0x" << std::hex << proc->violationAddress
                          << std::dec << " invalid.\n";
            }
            std::cout << "\n";
        }
        else if (command == "exit") inScreen = false;
        else std::cout << "Command not recognized. Use 'process-smi' or 'exit'.\n";
    }
}

// --- main menu -----------------------------------------------------------
int main() {
    Scheduler scheduler;
    Config    config;
    bool initialized = false;
    bool running = true;

    displayHeader();

    while (running) {
        std::cout << "root:\\> ";
        std::string line;
        if (!std::getline(std::cin, line)) break;

        std::stringstream ss(line);
        std::string action;
        ss >> action;

        if (action == "initialize") {
            if (initialized) { std::cout << "System is already initialized.\n"; continue; }
            if (!config.loadConfigFile("config.txt")) {
                std::cout << "Failed to initialize due to config file error.\n"; continue;
            }
            scheduler.initialize(config);
            scheduler.start();
            initialized = true;
            std::cout << "System initialized successfully.\n";
        }
        else if (action == "exit") {
            if (initialized) scheduler.stop();
            running = false;
        }
        else if (!initialized) {
            std::cout << "Please initialize the system first.\n";
        }

        // ---- scheduler-control commands ----
        else if (action == "scheduler-start") {
            scheduler.startBatchGeneration();
            std::cout << "Scheduler process generation started.\n";
        }
        else if (action == "scheduler-stop") {
            scheduler.stopBatchGeneration();
            std::cout << "Scheduler process generation stopped.\n";
        }
        else if (action == "report-util") {
            if (scheduler.saveReportToFile("csopesy-log.txt"))
                std::cout << "Report generated and saved to csopesy-log.txt successfully!\n";
            else
                std::cout << "Error: Could not write to csopesy-log.txt\n";
        }

        // ---- memory inspection commands ----
        else if (action == "vmstat") {
            std::cout << scheduler.buildVmstat();
        }
        else if (action == "process-smi") {
            std::cout << scheduler.buildProcessSmi();
        }

        // ---- screen commands ----
        else if (action == "screen") {
            std::string flag;
            ss >> flag;

            if (flag == "-s") {
                std::string procName;
                std::string memToken;
                ss >> procName >> memToken;

                std::uint64_t memSize = 0;
                if (procName.empty() || !parseU64(memToken, memSize) ||
                    !isValidMemorySize(memSize)) {
                    std::cout << "invalid memory allocation\n";
                    continue;
                }

                scheduler.createProcessWithMemory(procName,
                                                  static_cast<std::size_t>(memSize));
                Process* proc = scheduler.findProcess(procName);
                if (proc) {
                    clearScreen();
                    openProcessScreen(proc);
                    clearScreen();
                    displayHeader();
                }
            }
            else if (flag == "-c") {
                std::string procName;
                std::string memToken;
                ss >> procName >> memToken;

                std::uint64_t memSize = 0;
                if (procName.empty() || !parseU64(memToken, memSize) ||
                    !isValidMemorySize(memSize)) {
                    std::cout << "invalid memory allocation\n";
                    continue;
                }

                // The rest of the line is "<instructions>" (possibly quoted).
                std::string raw;
                std::getline(ss, raw);
                // trim leading whitespace
                std::size_t b = raw.find_first_not_of(" \t");
                if (b == std::string::npos) { std::cout << "invalid command\n"; continue; }
                raw = raw.substr(b);

                // strip surrounding quotes if present
                if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"')
                    raw = raw.substr(1, raw.size() - 2);

                std::vector<InstructionPtr> parsed;
                try {
                    parsed = parseUserInstructions(raw);
                } catch (const std::invalid_argument&) {
                    std::cout << "invalid command\n";
                    continue;
                }

                scheduler.createProcessWithMemory(
                    procName, static_cast<std::size_t>(memSize), std::move(parsed));

                Process* proc = scheduler.findProcess(procName);
                if (proc) {
                    clearScreen();
                    openProcessScreen(proc);
                    clearScreen();
                    displayHeader();
                }
            }
            else if (flag == "-r") {
                std::string procName;
                ss >> procName;

                if (procName.empty()) {
                    std::cout << "Error: Process name cannot be empty.\n";
                    continue;
                }
                Process* proc = scheduler.findProcess(procName);

                if (proc == nullptr || proc->state == ProcessState::FINISHED) {
                    std::cout << "Process " << procName << " not found.\n";
                }
                else if (proc->state == ProcessState::VIOLATED) {
                    std::cout << "Process " << procName
                              << " shut down due to memory access violation error "
                                 "that occurred at " << proc->violationTime
                              << ". 0x" << std::hex << proc->violationAddress
                              << std::dec << " invalid.\n";
                }
                else {
                    clearScreen();
                    openProcessScreen(proc);
                    clearScreen();
                    displayHeader();
                }
            }
            else if (flag == "-ls") {
                std::cout << scheduler.buildScreenList();
            }
            else {
                std::cout << "Invalid screen flag. Use -s, -c, -r, or -ls.\n";
            }
        }
        else {
            std::cout << "Unknown command.\n";
        }
    }
    return 0;
}