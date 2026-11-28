#include "process.h"
#include <chrono>
#include <iomanip>
#include <sstream>

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm timeInfo{};

#ifdef _WIN32
    localtime_s(&timeInfo, &time);
#else
    localtime_r(&time, &timeInfo);
#endif

    std::stringstream ss;
    ss << std::put_time(
        &timeInfo,
        "%m/%d/%Y %I:%M:%S%p"
    );

    return ss.str();
}

std::string getShortTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm timeInfo{};

#ifdef _WIN32
    localtime_s(&timeInfo, &time);
#else
    localtime_r(&time, &timeInfo);
#endif

    std::stringstream ss;
    ss << std::put_time(
        &timeInfo,
        "%H:%M:%S"
    );

    return ss.str();
}

Process::Process(std::string procName, int id, ProgramPtr program, std::size_t memoryBytes, DemandPagingAllocator& allocator) 
    : name(std::move(procName)),
      processId(id),
      totalInstructions(static_cast<int>(countInstructions(*program))),
      currentInstruction(0), 
      state(ProcessState::READY),
      createdAt(getCurrentTimestamp()),
      context(procName, program, id, memoryBytes, allocator),
      memoryRequired(memoryBytes) {
        numPages = allocator.processTotalPages(processId);
    }

bool Process::executeStep(int coreId) {
    if (state == ProcessState::FINISHED || state == ProcessState::VIOLATED) {
        return true;
    }

    if (context.isSleep()) {
        context.tickSleep();
        if (context.isDone()) {
            state = ProcessState::FINISHED;
            return true;
        }

        state = context.isSleep() ? ProcessState::WAITING : ProcessState::RUNNING;
        return false;
    }

    state = ProcessState::RUNNING;
    StepResult r;

    try {
        r = context.step();
    }
    catch (const MemoryAccessViolation& e) {
        violationAddress = e.address();
        violationTime = getShortTime(); // "HH:MM:SS"
        state = ProcessState::VIOLATED;
        currentInstruction = static_cast<int>(context.instructionsExecuted());
        pageFaults = context.pageFaults();
        residentPages = context.residentPages();

        return true;
    }

    currentInstruction = static_cast<int>(context.instructionsExecuted());
    pageFaults = context.pageFaults();
    residentPages = context.residentPages();

    if (r.hasOutput) {
        LogEntry log;
        log.timestamp = getCurrentTimestamp();
        log.coreId = coreId;
        log.message = r.output;
        logs.push_back(log);
    }
    if (r.sleepStarted) {
        state = ProcessState::WAITING;
    }
    if (r.done) {
        state = ProcessState::FINISHED;
        return true;
    }
    return false;
}