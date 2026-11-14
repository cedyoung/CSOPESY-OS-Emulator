#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool Config::loadConfigFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error: Could not open configuration file: " << filename << "\n";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string key;
        std::string value;

        if (ss >> key >> value) {
            
            if (value.front() == '"' && value.back() == '"' && value.length() >= 2) {
                value = value.substr(1, value.length() - 2);
            }

            if (key == "num-cpu") {
                numCpu = std::stoi(value);
            } else if (key == "scheduler") {
                scheduler = value;
            } else if (key == "quantum-cycles") {
                quantumCycles = std::stoi(value);
            } else if (key == "batch-process-freq") {
                batchProcessFreq = std::stoi(value);
            } else if (key == "min-ins") {
                minIns = std::stoi(value);
            } else if (key == "max-ins") {
                maxIns = std::stoi(value);
            } else if (key == "delays-per-exec") {
                delayPerExec = std::stoi(value);
            }
        }
    }

    file.close();
    if (numCpu < 1 || numCpu > 128) {
        std::cout << "Error: num-cpu must be between 1 and 128.\n";
        return false;
    }

    if (scheduler != "fcfs" && scheduler != "rr") {
        std::cout << "Error: scheduler must be fcfs or rr.\n";
        return false;
    }
    if (quantumCycles < 1 || batchProcessFreq < 1 || minIns < 1 || maxIns < minIns || delayPerExec < 0) {
        std::cout << "Error: Invalid configuration values.\n";
        return false;
    }

    return true;
}
