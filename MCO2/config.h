#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cstdint>

struct Config {
    int numCpu = 4;
    std::string scheduler = "rr";

    std::uint64_t quantumCycles = 5;
    std::uint64_t batchProcessFreq = 1;
    std::uint64_t minIns = 1000;
    std::uint64_t maxIns = 2000;
    std::uint64_t delayPerExec = 0;
    
    bool loadConfigFile(const std::string& filename);
};

#endif