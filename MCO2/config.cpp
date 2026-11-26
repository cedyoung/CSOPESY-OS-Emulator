#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdint>
#include <stdexcept>

namespace {

constexpr std::uint64_t MAX_CONFIG_VALUE =
    (1ULL << 32);

bool parseUnsigned(const std::string& text, std::uint64_t& result) {
    if (text.empty()) {
        return false;
    }

    if (text[0] == '-') {
        return false;
    }

    try {
        std::size_t pos = 0;

        unsigned long long value = std::stoull(text, &pos);

        if (pos != text.length()) {
            return false;
        }

        result = static_cast<std::uint64_t>(value);

        return true;
    }
    catch (const std::invalid_argument&) {
        return false;
    }
    catch (const std::out_of_range&) {
        return false;
    }
}
}

bool Config::loadConfigFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error: Could not open configuration file: " << filename << "\n";
        return false;
    }

    bool foundNumCpu = false;
    bool foundScheduler = false;
    bool foundQuantum = false;
    bool foundBatchFreq = false;
    bool foundMinIns = false;
    bool foundMaxIns = false;
    bool foundDelay = false;

    bool foundMaxOveralMem = false;
    bool foundMemPerFrame = false;
    bool foundMinMemPerProc = false;
    bool foundMaxMemPerProc = false;
    bool foundReplacement = false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string key;
        std::string value;

        if (!(ss >> key >> value)) {
            std::cout
                << "Error: Invalid configuration line: "
                << line
                << "\n";

            return false;
        }
        
        if (value.front() == '"' && value.back() == '"' && value.length() >= 2) {
            value = value.substr(1, value.length() - 2);
        }

        if (key == "num-cpu") {
            std::uint64_t temp;

            if (!parseUnsigned(value, temp)) {
                std::cout
                    << "Error: Invalid num-cpu value.\n";
                return false;
            }

            if (temp < 1 || temp > 128) {
                std::cout
                    << "Error: num-cpu must be between "
                    << "1 and 128.\n";
                return false;
            }

            numCpu = static_cast<int>(temp);
            foundNumCpu = true;
        }
        else if (key == "scheduler") {
            if (value != "fcfs" && value != "rr") {
                std::cout
                    << "Error: scheduler must be "
                    << "\"fcfs\" or \"rr\".\n";

                return false;
            }

            scheduler = value;
            foundScheduler = true;

        }
        else if (key == "quantum-cycles") {
            std::uint64_t temp;

            if (!parseUnsigned(value, temp)) {
                std::cout
                    << "Error: Invalid quantum-cycles value.\n";
                return false;
            }

            if (temp < 1 || temp > MAX_CONFIG_VALUE) {
                std::cout
                    << "Error: quantum-cycles must be between "
                    << "1 and 2^32.\n";
                return false;
            }

            quantumCycles = temp;
            foundQuantum = true;
        }
        else if (key == "batch-process-freq") {
            std::uint64_t temp;

            if (!parseUnsigned(value, temp)) {
                std::cout
                    << "Error: Invalid batch-process-freq value.\n";
                return false;
            }

            if (temp < 1 || temp > MAX_CONFIG_VALUE) {
                std::cout
                    << "Error: batch-process-freq must be between "
                    << "1 and 2^32.\n";
                return false;
            }

            batchProcessFreq = temp;
            foundBatchFreq = true;
        }
        else if (key == "min-ins") {
            std::uint64_t temp;

            if (!parseUnsigned(value, temp)) {
                std::cout
                    << "Error: Invalid min-ins value.\n";
                return false;
            }

            if (temp < 1 || temp > MAX_CONFIG_VALUE) {
                std::cout
                    << "Error: min-ins must be between "
                    << "1 and 2^32.\n";
                return false;
            }

            minIns = temp;
            foundMinIns = true;
        }
        else if (key == "max-ins") {
            std::uint64_t temp;

            if (!parseUnsigned(value, temp)) {
                std::cout
                    << "Error: Invalid max-ins value.\n";
                return false;
            }

            if (temp < 1 || temp > MAX_CONFIG_VALUE) {
                std::cout
                    << "Error: max-ins must be between "
                    << "1 and 2^32.\n";
                return false;
            }

            maxIns = temp;
            foundMaxIns = true;
        }
        else if (key == "delay-per-exec" || key == "delays-per-exec") {
            std::uint64_t temp;

            if (!parseUnsigned(value, temp)) {
                std::cout
                    << "Error: Invalid delay-per-exec value.\n";
                return false;
            }

            if (temp > MAX_CONFIG_VALUE) {
                std::cout
                    << "Error: delay-per-exec must be between "
                    << "0 and 2^32.\n";
                return false;
            }

            delayPerExec = temp;
            foundDelay = true;
        }
        

        else if (key == "demand-paging") {
            if (value != "true" && value != "false") {
                std::cout << "Error: demand-paging must be \"true\" or \"false\".\n";
                return false;
            }
            demandPaging = (value == "true");
            // foundDemand = true;   // uncomment if you made it mandatory
}

        else if (key == "replacement-policy") {
            if (value != "fifo" && value != "lru") {
                std::cout << "Error: replacement-policy must be \"fifo\" or \"lru\".\n";
                return false;
            }
            replacementPolicy = value;
            foundReplacement = true;
        }

        else if (key == "max-overall-mem") {
            std::uint64_t temp;
            if (!parseUnsigned(value, temp)) {
                std::cout << "Error: Invalid max-overall-mem value.\n";
                return false;
            }
            maxOveralMem = temp;
            foundMaxOveralMem = true;
}

        else if (key == "mem-per-frame") {
            std::uint64_t temp;
            if (!parseUnsigned(value, temp)) {
                std::cout << "Error: Invalid mem-per-frame value.\n";
                return false;
            }
            memPerFrame = temp;
            foundMemPerFrame = true;
        }

        else if (key == "min-mem-per-proc") {
            std::uint64_t temp;
            if (!parseUnsigned(value, temp)) {
                std::cout << "Error: Invalid min-mem-per-proc value.\n";
                return false;
            }
            minMemPerProc     = temp;
            foundMinMemPerProc = true;
        }

        else if (key == "max-mem-per-proc") {
            std::uint64_t temp;
            if (!parseUnsigned(value, temp)) {
                std::cout << "Error: Invalid max-mem-per-proc value.\n";
                return false;
            }
            maxMemPerProc     = temp;
            foundMaxMemPerProc = true;
        }
        else {
            std::cout
                << "Error: Unknown configuration key: "
                << key
                << "\n";

            return false;
        }
        
    }

    file.close();
    if (!foundNumCpu || !foundScheduler || !foundQuantum || !foundBatchFreq || !foundMinIns || !foundMaxIns || !foundDelay) {
        std::cout
            << "Error: Configuration file is missing "
            << "one or more required parameters.\n";

        return false;
    }
    if (maxIns < minIns) {
        std::cout
            << "Error: max-ins cannot be less than min-ins.\n";

        return false;
    }

    // 1. required keys present?
    if (!foundMaxOveralMem || !foundMemPerFrame ||
        !foundMinMemPerProc || !foundMaxMemPerProc ||
        !foundReplacement /* || !foundDemand */) {
        std::cout << "Error: Configuration file is missing one or more required "
                 "memory parameters.\n";
        return false;
    }

    // 2. power-of-two check for the [64, 65536] memory ranges
    auto isPowerOfTwo = [](std::uint64_t v) {
        return v != 0 && (v & (v - 1)) == 0;
    };
    auto inRange = [&](std::uint64_t v) {
        return v >= 64 && v <= 65536 && isPowerOfTwo(v);
    };

    if (!inRange(maxOveralMem)) {
        std::cout << "Error: max-overall-mem must be a power of 2 in [64, 65536].\n";
        return false;
    }
    if (!inRange(memPerFrame)) {
        std::cout << "Error: mem-per-frame must be a power of 2 in [64, 65536].\n";
        return false;
    }
    if (!inRange(minMemPerProc) || !inRange(maxMemPerProc)) {
        std::cout << "Error: min/max-mem-per-proc must be powers of 2 in [64, 65536].\n";
        return false;
    }

    // 3. cross-field consistency
    if (memPerFrame > maxOveralMem) {
        std::cout << "Error: mem-per-frame (" << memPerFrame
                  << ") exceeds max-overall-mem (" << maxOveralMem << ").\n";
        return false;
    }
    if (maxOveralMem % memPerFrame != 0) {
        std::cout << "Error: max-overall-mem must be divisible by mem-per-frame.\n";
        return false;
    }
    if (minMemPerProc > maxMemPerProc) {
        std::cout << "Error: min-mem-per-proc cannot be greater than max-mem-per-proc.\n";
        return false;
    }
    return true;
}
