#include "scheduler.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

FCFSScheduler::FCFSScheduler() {
    coreBusy.fill(false);
    runningProcesses.fill(nullptr);

    std::filesystem::create_directories("process_logs");
}

void FCFSScheduler::start() {
    {

        std::lock_guard<std::mutex> lock(schedulerMutex);

        if (running) {
            return;
        }

        running = true;
    }

    
    for (int core = 0; core < CPU_CORES; ++core) {
        workerThreads[core] = std::thread(
            &FCFSScheduler::workerLoop,
            this,
            core
        );
    }

    
    schedulerThread = std::thread(&FCFSScheduler::schedulerLoop, this);
}

void FCFSScheduler::stop() {
    {

        std::lock_guard<std::mutex> lock(schedulerMutex);

        if (!running) {
            return;
        }

        running = false;
    }

    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }

    for (std::thread& worker : workerThreads) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

FCFSScheduler::~FCFSScheduler() {
    stop();

    for (Process* process : processes) {
        delete process;
    }
}

void FCFSScheduler::createProcess(const std::string& name, int commands) {
    Process* process = new Process;

    process->name = name;
    process->createdAt = timestampNow();
    process->finishedAt = "";
    process->totalCommands = commands;
    process->executedCommands.store(0);
    process->state = ProcessState::Ready;
    process->coreId = -1;

    {
        std::lock_guard<std::mutex> lock(schedulerMutex);

        processes.push_back(process);
        readyQueue.push(process);
    }

    std::ofstream logFile(
        "process_logs/" + name + ".txt",
        std::ios::trunc
    );

    if (logFile.is_open()) {
        logFile << "Process name: " << name << "\n";
        logFile << "Logs:\n";
    }
}

void FCFSScheduler::schedulerLoop() {

    while (true) {
        {
            std::lock_guard<std::mutex> lock(schedulerMutex);

            if (!running) {
                return;
            }

            
            while (!readyQueue.empty()) {
                int freeCore = -1;

                for (int core = 0; core < CPU_CORES; ++core) {

                    if (!coreBusy[core]) {
                        freeCore = core;
                        break;
                    }
                }

                if (freeCore == -1) {
                    break;
                }

                Process* process = readyQueue.front();
                readyQueue.pop();


                process->state = ProcessState::Running;
                process->coreId = freeCore;

                runningProcesses[freeCore] = process;
                coreBusy[freeCore] = true;
            }
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(POLL_INTERVAL_MS)
        );
    }
}

void FCFSScheduler::workerLoop(int coreId) {

    while (true) {
        Process* process = nullptr;

        {
            std::lock_guard<std::mutex> lock(schedulerMutex);

            if (!running) {
                return;
            }

            if (coreBusy[coreId]) {
                process = runningProcesses[coreId];
            }
        }

        if (process == nullptr) {
        
            std::this_thread::sleep_for(
                std::chrono::milliseconds(POLL_INTERVAL_MS)
            );
            continue;
        }

        
        executeProcess(process, coreId);

        
        if (process->executedCommands.load() < process->totalCommands) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(schedulerMutex);

            process->state = ProcessState::Finished;
            process->finishedAt = timestampNow();
            process->coreId = -1;

            runningProcesses[coreId] = nullptr;
            coreBusy[coreId] = false;
        }
    }
}

void FCFSScheduler::executeProcess(
    Process* process,
    int coreId
) {
    Process* process,
        int coreId
        ) {
            std::ofstream logFile;

            if (WRITE_LOG_FILES) {
                logFile.open(
                    "process_logs/" + process->name + ".txt",
                    std::ios::app
                );
            }

            while (process->executedCommands.load() < process->totalCommands) {
                {
                    std::lock_guard<std::mutex> lock(schedulerMutex);

                    if (!running) {
                        return;\
                    }
                }

                
                if (logFile.is_open()) {
                    logFile
                        << "(" << timestampNow() << ") "
                        << "Core:" << coreId << " "
                        << "\"Hello world from " << process->name << "!\"\n";
                }

                process->executedCommands.fetch_add(1);

                
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(EXEC_DELAY_MS)
                );
            }
}

void FCFSScheduler::printScreenList() {
    // TODO:
    // Print:
    //
    // Running processes:
    // process_01 (...) Core: 0   50 / 100
    //
    // Finished processes:
    // process_02 (...) Finished  100 / 100
}

bool FCFSScheduler::allFinished() {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    if (processes.empty()) {
        return false;
    }

    for (Process* process : processes) {
        if (process->state != ProcessState::Finished) {
            return false;
        }
    }

    return true;
}

std::string FCFSScheduler::timestampNow() const {
    const std::time_t now = std::time(nullptr);
    std::tm localTime{};

#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    std::ostringstream output;

    output
        << std::put_time(
            &localTime,
            "%m/%d/%Y %I:%M:%S%p"
        );

    return output.str();
}
