#ifndef FCFS_SCHEDULER_H
#define FCFS_SCHEDULER_H

#include "config.h"
#include "process.h"
#include "memory.h"
#include "backing_store.h"

#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <cstdint>
#include <string>
#include <condition_variable>

class Scheduler {
public:
    Scheduler();
    ~Scheduler();

    void initialize(const Config& config);
    void start();
    void stop();

    
    std::string buildScreenList();

    std::string buildVmstat();
    std::string buildProcessSmi();

    Process* findProcess(const std::string& name);

    bool saveReportToFile(const std::string& filename = "csopesy-log.txt");

    void startBatchGeneration();
    void stopBatchGeneration();


    

    void createProcessWithMemory(const std::string& name,
                             std::size_t memoryBytes,
                             std::vector<InstructionPtr> userInstructions = {});
    
    
private:
    void schedulerLoop();
    void workerLoop(int coreId);

    void dispatchFCFS();
    void dispatchRoundRobin();
    int findFreeCore() const;

    void finishProcess(int processIndex, int coreId);
    void requeueProcess(int processIndex, int coreId);
    void sleepProcess(int processIndex, int coreId);
    void finishSleepingProcess();

    void generateBatchProcessIfNeeded();
    std::string nextGeneratedProcessName();

    Config config;
    int nextProcessId = 1;

    std::vector<std::shared_ptr<Process>> processes;
    std::queue<int> readyQueue;
    std::vector<int> sleepingProcess;
    std::vector<int> runningProcess;
    std::vector<bool> coreBusy;

    std::vector<std::uint64_t> lastHandledTick;
    std::vector<std::uint64_t> rrCyclesUsed;
    
    std::thread schedulerThread;
    std::vector<std::thread> workerThreads;

    mutable std::mutex schedulerMutex;
    std::condition_variable tickCondition;

    std::unique_ptr<BackingStore> backingStore_;
    std::unique_ptr<DemandPagingAllocator> allocator_;
    std::uint64_t activeCpuTicks_ = 0;

    std::uint64_t cpuTicks = 0;

    bool initialized = false;
    bool running = false;

    bool batchGenerationEnabled = false;
    int nextGeneratedProcessNumber = 1;
};

#endif
