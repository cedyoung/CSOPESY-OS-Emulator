#include "scheduler.h"

#include <chrono>
#include <random>
#include <iostream>
#include <iomanip>
#include <sstream>

Scheduler::Scheduler() = default;

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::initialize(const Config& newConfig) {
    std::lock_guard<std::mutex> lock(
        schedulerMutex
    );

    if (running) {
        return;
    }

    config = newConfig;

    runningProcess.resize(
        config.numCpu,
        -1
    );

    coreBusy.resize(
        config.numCpu,
        false
    );

    initialized = true;
}

void Scheduler::start() {
    {
        std::lock_guard<std::mutex> lock(schedulerMutex);

        if (!initialized || running) {
            return;
        }

        running = true;
    }

    workerThreads.clear();

    for (int core = 0; core < config.numCpu; ++core) {
        workerThreads.emplace_back(
            &Scheduler::workerLoop,
            this,
            core
        );
    }

    schedulerThread = std::thread(
        &Scheduler::schedulerLoop,
        this
    );
}

void Scheduler::stop() {
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

    workerThreads.clear();
}

void Scheduler::schedulerLoop() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(schedulerMutex);

            if (!running) {
                return;
            }
            
            cpuTicks++;
            generateBatchProcessIfNeeded();

            if (config.scheduler == "fcfs") {
                dispatchFCFS();
            }
            else if (config.scheduler == "rr") {
                dispatchRoundRobin();
            }
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(5)
        );
    }
}

void Scheduler::workerLoop(int coreId) {
    while (true) {

        int processIndex = -1;

        {
            std::lock_guard<std::mutex> lock(
                schedulerMutex
            );

            if (!running) {
                return;
            }

            if (coreBusy[coreId]) {
                processIndex =
                    runningProcess[coreId];
            }
        }

        if (processIndex == -1) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(5)
            );

            continue;
        }


        if (config.scheduler == "fcfs") {

            bool finished;
            {
                std::lock_guard<std::mutex> stepLock(schedulerMutex);
                finished = processes[processIndex].executeStep(coreId);
            }

            if (finished) {
                std::lock_guard<std::mutex> lock(
                    schedulerMutex
                );

                finishProcess(
                    processIndex,
                    coreId
                );
            }
        }


        else if (config.scheduler == "rr") {

            bool finished = false;

            for (int cycle = 0;
                 cycle < config.quantumCycles;
                 ++cycle) {

                {
                    std::lock_guard<std::mutex> stepLock(schedulerMutex);
                    finished = processes[processIndex].executeStep(coreId);
                }

                if (finished) {
                    break;
                }
            }

            std::lock_guard<std::mutex> lock(
                schedulerMutex
            );

            if (finished) {
                finishProcess(processIndex, coreId);
            }
            else {
                requeueProcess(processIndex, coreId);
            }
        }
        std::this_thread::sleep_for(
            std::chrono::milliseconds(5)
        );
    }
}

void Scheduler::dispatchFCFS() {
    while (!readyQueue.empty()) {

        int freeCore = findFreeCore();

        if (freeCore == -1) {
            return;
        }

        int processIndex =
            readyQueue.front();

        readyQueue.pop();

        processes[processIndex].state =
            ProcessState::RUNNING;

        runningProcess[freeCore] =
            processIndex;

        coreBusy[freeCore] = true;
    }
}

void Scheduler::dispatchRoundRobin() {
    while (!readyQueue.empty()) {

        int freeCore = findFreeCore();

        if (freeCore == -1) {
            return;
        }

        int processIndex =
            readyQueue.front();

        readyQueue.pop();

        processes[processIndex].state =
            ProcessState::RUNNING;

        runningProcess[freeCore] =
            processIndex;

        coreBusy[freeCore] = true;
    }
}

int Scheduler::findFreeCore() const {
    for (int core = 0;
         core < config.numCpu;
         ++core) {

        if (!coreBusy[core]) {
            return core;
        }
    }

    return -1;
}

void Scheduler::finishProcess(int processIndex, int coreId) {
    processes[processIndex].state =
        ProcessState::FINISHED;

    runningProcess[coreId] = -1;
    coreBusy[coreId] = false;
}

void Scheduler::requeueProcess(int processIndex, int coreId) {
    processes[processIndex].state =
        ProcessState::READY;

    runningProcess[coreId] = -1;
    coreBusy[coreId] = false;

    readyQueue.push(processIndex);
}

void Scheduler::createProcess(const std::string& name) {
    std::lock_guard<std::mutex> lock(schedulerMutex);


    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(config.minIns, config.maxIns);
    int totalInstructions = dis(gen);


    Process newProc(name, nextProcessId++, generateProgram(name, totalInstructions, gen));
    processes.push_back(newProc);

    int processIndex =
    static_cast<int>(
        processes.size()
    ) - 1;

    readyQueue.push(processIndex);

    std::cout
        << "Process '" << name
        << "' created with ID "
        << newProc.processId
        << " and "
        << totalInstructions
        << " instructions.\n";
}

Process* Scheduler::findProcess(const std::string& name) {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    for (auto& proc : processes) {
        if (proc.name == name) {
            return &proc;
        }
    }
    return nullptr;
}

std::string Scheduler::buildScreenList() {
    std::lock_guard<std::mutex> lock(
        schedulerMutex
    );

    std::ostringstream output;

    int usedCores = 0;

    for (bool busy : coreBusy) {
        if (busy) {
            usedCores++;
        }
    }

    int availableCores =
        config.numCpu - usedCores;

    double cpuUtilization = 0.0;

    if (config.numCpu > 0) {
        cpuUtilization =
            (static_cast<double>(usedCores)
             / config.numCpu)
            * 100.0;
    }


    output << "\n----------------------------------\n";

    output
        << "CPU utilization: "
        << std::fixed
        << std::setprecision(2)
        << cpuUtilization
        << "%\n";

    output
        << "Cores used: "
        << usedCores
        << "\n";

    output
        << "Cores available: "
        << availableCores
        << "\n";


    output << "\nRunning processes:\n";

    bool hasRunning = false;

    for (const Process& process : processes) {

        if (process.state == ProcessState::RUNNING || process.state == ProcessState::WAITING) {

            hasRunning = true;

            output
                << std::left
                << std::setw(15)
                << process.name
                << " ID: "
                << process.processId
                << "  "
                << process.currentInstruction
                << " / "
                << process.totalInstructions
                << "\n";
        }
    }

    if (!hasRunning) {
        output << "None\n";
    }


    output << "\nFinished processes:\n";

    bool hasFinished = false;

    for (const Process& process : processes) {

        if (process.state == ProcessState::FINISHED) {

            hasFinished = true;

            output
                << std::left
                << std::setw(15)
                << process.name
                << " ID: "
                << process.processId
                << "  Finished  "
                << process.currentInstruction
                << " / "
                << process.totalInstructions
                << "\n";
        }
    }

    if (!hasFinished) {
        output << "None\n";
    }


    output << "----------------------------------\n";

    return output.str();
}

void Scheduler::startBatchGeneration() {
    std::lock_guard<std::mutex> lock(
        schedulerMutex
    );

    batchGenerationEnabled = true;
}

void Scheduler::stopBatchGeneration() {
    std::lock_guard<std::mutex> lock(
        schedulerMutex
    );

    batchGenerationEnabled = false;
}

std::string Scheduler::nextGeneratedProcessName() {
    std::ostringstream output;

    output
        << "screen_"
        << std::setw(2)
        << std::setfill('0')
        << nextGeneratedProcessNumber++;

    return output.str();
}

void Scheduler::generateBatchProcessIfNeeded() {

    if (!batchGenerationEnabled) {
        return;
    }

    if (config.batchProcessFreq <= 0) {
        return;
    }

    if (
        cpuTicks %
        static_cast<std::uint64_t>(
            config.batchProcessFreq
        )
        != 0
    ) {
        return;
    }

    std::string name =
        nextGeneratedProcessName();

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> dis(
        config.minIns,
        config.maxIns
    );

    int totalInstructions = dis(gen);


    Process newProc(
        name,
        nextProcessId++,
        generateProgram(name, totalInstructions, gen)
    );

    processes.push_back(newProc);


    int processIndex =
        static_cast<int>(
            processes.size()
        ) - 1;

    readyQueue.push(processIndex);
}
