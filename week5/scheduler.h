#ifndef FCFS_SCHEDULER_H
#define FCFS_SCHEDULER_H

#include <array>
#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

enum class ProcessState {
    Ready,
    Running,
    Finished
};

struct Process {
    std::string name;
    std::string createdAt;
    std::string finishedAt;

    int totalCommands = 0;
    std::atomic<int> executedCommands{0};

    ProcessState state = ProcessState::Ready;
    int coreId = -1;
};

class FCFSScheduler {
public:
    static constexpr int CPU_CORES = 4;
    static constexpr int POLL_INTERVAL_MS = 5;
    static constexpr int EXEC_DELAY_MS = 50;
    static constexpr bool WRITE_LOG_FILES = true;

    FCFSScheduler();
    ~FCFSScheduler();

    void createProcess(const std::string& name, int commands);
    void start();
    void stop();
    void printScreenList();
    bool allFinished();

private:
    void schedulerLoop();
    void workerLoop(int coreId);
    void executeProcess(Process* process, int coreId);

    std::string timestampNow() const;

    std::vector<Process*> processes;
    std::queue<Process*> readyQueue;

    std::thread schedulerThread;
    std::array<std::thread, CPU_CORES> workerThreads;

    std::array<Process*, CPU_CORES> runningProcesses{};
    std::array<bool, CPU_CORES> coreBusy{};

    std::mutex schedulerMutex;

    bool running = false;
};

#endif
