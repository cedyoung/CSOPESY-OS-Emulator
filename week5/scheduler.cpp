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
    // TODO: scheduler and worker thread startup.
}

void FCFSScheduler::stop() {
    // TODO: thread stopping logic.
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
    // TODO:
    // 1. Continuously check the readyQueue
    // 2. Check which CPU cores are free
    // 3. Take the process at readyQueue.front()
    // 4. readyQueue.pop()
    // 5. Change process state to Running
    // 6. Assign its coreId
    // 7. Store it in runningProcesses[core]
    // 8. Mark coreBusy[core] = true
}

void FCFSScheduler::workerLoop(int coreId) {
    // TODO:
    // 1. Check if this core has an assigned process
    // 2. Call executeProcess(process, coreId)
    // 3. When finished:
    //      state = Finished
    //      finishedAt = timestampNow()
    //      coreId = -1
    // 4. Set runningProcesses[coreId] = nullptr
    // 5. Set coreBusy[coreId] = false
}

void FCFSScheduler::executeProcess(
    Process* process,
    int coreId
) {
    // TODO:
    // 1. Open the process text file
    // 2. Loop through all 100 commands
    // 3. Write:
    //      timestamp
    //      CPU core
    //      print message
    // 4. Update executedCommands
    // 5. Add simulation delay if needed
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
