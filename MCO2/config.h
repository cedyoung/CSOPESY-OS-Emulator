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
    std::uint64_t maxOveralMem = 16384;

    std::uint64_t memPerFrame = 64;
    std::uint64_t minMemPerProc = 64;
    std::uint64_t maxMemPerProc = 4096;
    bool demandPaging = true;
    std::string replacementPolicy = "fifo";
    
    bool loadConfigFile(const std::string& filename);
};

#endif