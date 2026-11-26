#include "process.h"
#include <chrono>
#include <iomanip>
#include <sstream>

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%m/%d/%Y %I:%M:%S%p");
    return ss.str();
}

std::string getShortTime() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%H:%M:%S");
    return ss.str();
}


Process::Process(std::string procName, int id, ProgramPtr program, std::size_t memoryBytes, DemandPagingAllocator& allocator) 
    : name(procName), 
      processId(id), 
      totalInstructions(static_cast<int>(countInstructions(*program))), 
      currentInstruction(0), 
      state(ProcessState::READY),
      createdAt(getCurrentTimestamp()),
      context(procName, program, id, memoryBytes, allocator),
      memoryRequired(memoryBytes),
      numPages((memoryBytes + 63) / 64) {
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
    StepResult r = context.step();
    try {
        r = context.step();
    } catch (const MemoryAccessViolation& e) {
        violationAddress = e.address();
        violationTime = getShortTime(); // "HH:MM:SS"
        state = ProcessState::VIOLATED;
        currentInstruction = static_cast<int>(context.instructionsExecuted());
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
    if (r.sleepStarted) state = ProcessState::WAITING;
    if (r.done) { state = ProcessState::FINISHED; return true; }
    return false;
}