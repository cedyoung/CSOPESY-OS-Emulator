#include "scheduler.h"

#include <chrono>
#include <random>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

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

    coreBusy.assign(
        config.numCpu,
        false
    );

    runningProcess.assign(
        config.numCpu,
        -1
    );

    lastHandledTick.assign(
        config.numCpu,
        0
    );

    rrCyclesUsed.assign(
        config.numCpu,
        0
    );

    cpuTicks = 0;

    backingStore_ = std::make_unique<BackingStore>(
    static_cast<std::size_t>(config.memPerFrame));

    auto policy = (config.replacementPolicy == "lru")
                  ? DemandPagingAllocator::Policy::LRU
                  : DemandPagingAllocator::Policy::FIFO;

    allocator_ = std::make_unique<DemandPagingAllocator>(
        *backingStore_,
        static_cast<std::size_t>(config.maxOveralMem),
        static_cast<std::size_t>(config.memPerFrame),
        policy);

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

    tickCondition.notify_all();
    
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
            
            ++cpuTicks;
            generateBatchProcessIfNeeded();
            finishSleepingProcess();

            if (config.scheduler == "fcfs") {
                dispatchFCFS();
            }
            else if (config.scheduler == "rr") {
                dispatchRoundRobin();
            }
        }

        tickCondition.notify_all();

        std::this_thread::sleep_for(
            std::chrono::milliseconds(500)
        );
    }
}

void Scheduler::workerLoop(int coreId) {
    std::unique_lock<std::mutex> lock(schedulerMutex);
    while (true) {
        tickCondition.wait(lock, [&]() {
            return !running ||
                   cpuTicks > lastHandledTick[coreId];
        });

        if (!running) {
            return;
        }

        const std::uint64_t currentTick = cpuTicks;
        lastHandledTick[coreId] = currentTick;

        if (!coreBusy[coreId]) {
            continue;
        }

        const int processIndex = runningProcess[coreId];
        Process& process = *processes[processIndex];

        if (coreBusy[coreId]) ++activeCpuTicks_;

        bool finished = false;

        const bool wasSleeping = process.context.isSleep();

        const bool instructionReady =
            wasSleeping ||
            currentTick >= process.nextExecutionTick;

        if (instructionReady) {
            const int instructionBefore =
                process.currentInstruction;

            finished = process.executeStep(coreId);

            if (process.currentInstruction > instructionBefore) {
                process.nextExecutionTick =
                    currentTick
                    + static_cast<std::uint64_t>(
                        config.delayPerExec
                    )
                    + 1;
            }
        }
        if (config.scheduler == "rr") {
            ++rrCyclesUsed[coreId];
        }

        if (finished) {
            finishProcess(processIndex, coreId);
        }
        else if (process.context.isSleep()){
            sleepProcess(processIndex, coreId);
        }
        else if (
            config.scheduler == "rr" &&
            rrCyclesUsed[coreId] >=
                static_cast<std::uint64_t>(
                    config.quantumCycles
                )
        ) {
            requeueProcess(processIndex, coreId);
        }
    }
}

void Scheduler::sleepProcess(int processIndex, int coreId){
    processes[processIndex]->state = ProcessState::WAITING;

    runningProcess[coreId] = -1;
    coreBusy[coreId] = false;
    rrCyclesUsed[coreId] = 0;

    sleepingProcess.push_back(processIndex);
    
    
}

void Scheduler::finishSleepingProcess() {
    for (auto iterate = sleepingProcess.begin();
        iterate != sleepingProcess.end();) {
        Process& process = *processes[*iterate];
            process.context.tickSleep();


            
            if (process.context.isSleep()) {
                ++iterate;
                continue;
            }
            if (process.context.isDone()) {
                process.state = ProcessState::FINISHED;
            } else {
                process.state = ProcessState::READY;
                readyQueue.push(*iterate);
            }
            iterate = sleepingProcess.erase(iterate);
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

        processes[processIndex]->state =
            ProcessState::RUNNING;

        runningProcess[freeCore] =
            processIndex;

        coreBusy[freeCore] = true;
        rrCyclesUsed[freeCore] = 0;
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

        processes[processIndex]->state =
            ProcessState::RUNNING;

        runningProcess[freeCore] =
            processIndex;

        coreBusy[freeCore] = true;
        rrCyclesUsed[freeCore] = 0;
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
    // preserve VIOLATED — only flip to FINISHED if not already violated
    if (processes[processIndex]->state != ProcessState::VIOLATED)
        processes[processIndex]->state = ProcessState::FINISHED;

    allocator_->destroyProcess(processes[processIndex]->processId);

    runningProcess[coreId] = -1;
    coreBusy[coreId]       = false;
    rrCyclesUsed[coreId]   = 0;
}

void Scheduler::requeueProcess(int processIndex, int coreId) {
    processes[processIndex]->state =
        ProcessState::READY;

    runningProcess[coreId] = -1;
    coreBusy[coreId] = false;
    rrCyclesUsed[coreId] = 0;

    readyQueue.push(processIndex);
}



void Scheduler::createProcessWithMemory(
    const std::string& name,
    std::size_t memoryBytes,
    std::vector<InstructionPtr> userInstructions) {

    std::lock_guard<std::mutex> lock(schedulerMutex);

    if (memoryBytes < 64 || memoryBytes > config.maxOveralMem ||
        (memoryBytes & (memoryBytes - 1)) != 0) {
        std::cout << "invalid memory allocation\n";
        return;
    }

    const int pid = nextProcessId;

    if (!allocator_->createProcess(pid, memoryBytes)) {
        std::cout << "invalid memory allocation\n";
        return;
    }
    ++nextProcessId;

    ProgramPtr program;
    if (!userInstructions.empty()) {
        // Trust the caller: 1..50 instructions already validated
        program = std::make_shared<const InstructionList>(
            std::move(userInstructions));
    } else {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<std::uint64_t> dis(
            config.minIns, config.maxIns);
        program = generateProgram(name, dis(gen), gen, memoryBytes);
    }

    auto proc = std::make_shared<Process>(
        name, pid, program, memoryBytes, *allocator_);

    processes.push_back(proc);
    readyQueue.push(static_cast<int>(processes.size()) - 1);

    std::cout << "Process '" << name << "' created with ID " << pid
              << " (" << memoryBytes << " B, " << proc->numPages
              << " pages).\n";
}

Process* Scheduler::findProcess(const std::string& name) {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    for (auto& proc : processes) {
        if (proc->name == name) {
            return proc.get();
        }
    }
    return nullptr;
}

std::string Scheduler::buildScreenList() {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    std::ostringstream output;

    int usedCores = 0;
    for (bool busy : coreBusy) {
        if (busy) {
            usedCores++;
        }
    }

    int availableCores = config.numCpu - usedCores;
    double cpuUtilization = 0.0;
    if (config.numCpu > 0) {
        cpuUtilization = (static_cast<double>(usedCores) / config.numCpu) * 100.0;
    }

    output << "CPU utilization: " << std::fixed << std::setprecision(0) << cpuUtilization << "%\n";
    output << "Cores used: " << usedCores << "\n";
    output << "Cores available: " << availableCores << "\n";
    output << "\n--------------------------------------------------------------------\n";


    output << "Running processes:\n";
    bool hasRunning = false;
    for (const auto& processPtr : processes) {
        const Process& process = *processPtr;
        if (process.state == ProcessState::RUNNING || process.state == ProcessState::WAITING || process.state == ProcessState::READY) {
            hasRunning = true;

            int assignedCore = -1;
            for (int c = 0; c < config.numCpu; ++c) {
                if (runningProcess[c] != -1 && processes[runningProcess[c]]->processId == process.processId) {
                    assignedCore = c;
                    break;
                }
            }

            output << std::left << std::setw(12) << process.name
                   << " (" << process.createdAt << ")  ";
            
            if (assignedCore != -1) {
                output << "Core: " << assignedCore << "    ";
            } else {
                output << "Core: N/A  ";
            }

            output << process.currentInstruction << " / " << process.totalInstructions << "\n";
        }
    }
    if (!hasRunning) {
        output << "None\n";
    }

    output << "\nFinished processes:\n";
    bool hasFinished = false;
    for (const auto& processPtr : processes) {
        const Process& process = *processPtr;
        if (process.state == ProcessState::FINISHED) {
            hasFinished = true;

            output << std::left << std::setw(12) << process.name
                   << " (" << process.createdAt << ")  "
                   << std::left << std::setw(10) << "Finished"
                   << process.currentInstruction << " / " << process.totalInstructions << "\n";
        }
    }
    if (!hasFinished) {
        output << "None\n";
    }

    output << "--------------------------------------------------------------------\n";

    return output.str();
}

std::string Scheduler::buildVmstat() {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    std::ostringstream out;

    const std::uint64_t totalTicks =
        cpuTicks * static_cast<std::uint64_t>(config.numCpu);
    const std::uint64_t idleTicks  = totalTicks - activeCpuTicks_;

    out << totalTicks        << "\ttotal cpu ticks\n";
    out << activeCpuTicks_   << "\tactive cpu ticks\n";
    out << idleTicks         << "\tidle cpu ticks\n";
    out << allocator_->totalMemory()   << " B\ttotal memory\n";
    out << allocator_->usedMemory()    << " B\tused memory\n";
    out << allocator_->freeMemory()    << " B\tfree memory\n";
    out << allocator_->pagesPagedIn()  << "\tpages paged in\n";
    out << allocator_->pagesPagedOut() << "\tpages paged out\n";

    return out.str();
}

std::string Scheduler::buildProcessSmi() {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    std::ostringstream out;

    out << "| PROCESS-SMI V01.00 Driver Version: 01.00\n";
    out << "------------------------------------------\n";

    int coresBusy = 0;
    for (bool b : coreBusy) if (b) ++coresBusy;
    const int util = config.numCpu > 0
                   ? coresBusy * 100 / config.numCpu : 0;

    const std::size_t usedFrames  = allocator_->usedFrames();
    const std::size_t totalFrames = allocator_->totalFrames();

    out << "CPU-Util: " << util << "%\n";
    out << "Memory Usage: " << allocator_->usedMemory()
        << " B / " << allocator_->totalMemory() << " B\n";
    out << "Memory Util: "
        << (totalFrames ? usedFrames * 100 / totalFrames : 0) << "%\n";
    out << "------------------------------------------\n";
    out << "Running processes and memory usage:\n";
    out << "------------------------------------------\n";

    for (auto& p : processes) {
        if (p->state == ProcessState::RUNNING ||
            p->state == ProcessState::READY   ||
            p->state == ProcessState::WAITING) {
            out << p->name << "  " << p->memoryRequired << " B\n";
        }
    }
    return out.str();
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
    if (!batchGenerationEnabled) return;
    if (config.batchProcessFreq == 0) return;
    if (cpuTicks % config.batchProcessFreq != 0) return;

    std::string name = nextGeneratedProcessName();

    std::vector<std::uint64_t> sizes;
    for (std::uint64_t m = config.minMemPerProc;
         m <= config.maxMemPerProc; m <<= 1)
        sizes.push_back(m);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> pick(0, sizes.size() - 1);

    createProcessWithMemory(name,
        static_cast<std::size_t>(sizes[pick(gen)]));
}

bool Scheduler::saveReportToFile(const std::string& filename) {
    std::string reportContent = buildScreenList();
    
    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) {
        return false;
    }

    file << reportContent << "\n";
    file.close();
    return true;
}
