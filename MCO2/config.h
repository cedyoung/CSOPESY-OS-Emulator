#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
    int numCpu = 4;
    std::string scheduler = "rr";
    int quantumCycles = 5;
    int batchProcessFreq = 1;
    int minIns = 1000;
    int maxIns = 2000;
    int delayPerExec = 0;

    
    bool loadConfigFile(const std::string& filename);
};

#endif