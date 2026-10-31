#ifndef FCFS_SCHEDULER_H
#define FCFS_SCHEDULER_H

#include "config.h"

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

private:
    void schedulerLoop();
    void workerLoop(int coreId);

    Config config;

    std::thread schedulerThread;
    std::vector<std::thread> workerThreads;

    std::mutex schedulerMutex;

    bool initialized = false;
    bool running = false;
};

#endif
