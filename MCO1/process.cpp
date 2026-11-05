#include "process.h"
#include <chrono>
#include <iomanip>
#include <sstream>

Process::Process(std::string procName, int id, ProgramPtr program) 
    : name(procName), 
      processId(id), 
      totalInstructions(static_cast<int>(countInstructions(*program))), 
      currentInstruction(0), 
      state(ProcessState::READY),
      context(procName, program) {
}


std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%m/%d/%Y %I:%M:%S%p");
    return ss.str();
}


bool Process::executeStep(int coreId) {
    if (state == ProcessState::FINISHED) {
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
    currentInstruction = static_cast<int>(context.instructionsExecuted());

    if (r.hasOutput){
        LogEntry log;
        log.timestamp = getCurrentTimestamp();
        log.coreId = coreId;
        log.message = r.output;
        logs.push_back(log);
    }

    if (r.sleepStarted){
        state = ProcessState::WAITING;
    }
    
    if (r.done) {
        state = ProcessState::FINISHED;
        return true;
    }

    return false;
}
