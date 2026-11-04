#include "process.h"
#include <chrono>
#include <iomanip>
#include <sstream>

Process::Process(std::string procName, int id, int totalInst) 
    : name(procName), 
      processId(id), 
      totalInstructions(totalInst), 
      currentInstruction(0), 
      state(ProcessState::READY) {
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

    state = ProcessState::RUNNING;
    currentInstruction++;

    LogEntry log;
    log.timestamp = getCurrentTimestamp();
    log.coreId = coreId;
    log.message = "Hello World from " + name + "!";
    logs.push_back(log);

    if (currentInstruction >= totalInstructions) {
        state = ProcessState::FINISHED;
        return true;
    }

    return false;
}