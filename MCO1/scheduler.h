#ifndef FCFS_SCHEDULER_H
#define FCFS_SCHEDULER_H

#include "config.h"
#include "process.h"

#include <mutex>
#include <thread>
#include <vector>

class FCFSScheduler {
public:
    FCFSScheduler();
    ~FCFSScheduler();

    void initialize(const Config& config);
    void start();
    void stop();

    void createProcess(const std::string& name);
    std::vector<Process> getActiveProcesses();

    Process* findProcess(const std::string& name);
    
private:
    void schedulerLoop();
    void workerLoop(int coreId);

    Config config;
    int nextProcessId = 1;

    std::vector<Process> processes;

    std::thread schedulerThread;
    std::vector<std::thread> workerThreads;

    std::mutex schedulerMutex;

    bool initialized = false;
    bool running = false;

    
};

#endif
