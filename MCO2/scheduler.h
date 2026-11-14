#ifndef FCFS_SCHEDULER_H
#define FCFS_SCHEDULER_H

#include "config.h"
#include "process.h"

#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <cstdint>
#include <string>

class Scheduler {
public:
    Scheduler();
    ~Scheduler();

    void initialize(const Config& config);
    void start();
    void stop();

    void createProcess(const std::string& name);
    
    std::string buildScreenList();

    Process* findProcess(const std::string& name);

    bool saveReportToFile(const std::string& filename = "csopesy-log.txt");

    void startBatchGeneration();
    void stopBatchGeneration();
    
private:
    void schedulerLoop();
    void workerLoop(int coreId);

    void dispatchFCFS();
    void dispatchRoundRobin();
    int findFreeCore() const;

    void finishProcess(int processIndex, int coreId);
    void requeueProcess(int processIndex, int coreId);

    void generateBatchProcessIfNeeded();
    std::string nextGeneratedProcessName();

    Config config;
    int nextProcessId = 1;

    std::vector<Process> processes;
    std::queue<int> readyQueue;
    std::vector<int> runningProcess;
    std::vector<bool> coreBusy;

    std::thread schedulerThread;
    std::vector<std::thread> workerThreads;

    std::mutex schedulerMutex;

    std::uint64_t cpuTicks = 0;

    bool initialized = false;
    bool running = false;

    bool batchGenerationEnabled = false;
    int nextGeneratedProcessNumber = 1;
};

#endif
