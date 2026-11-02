#include "process.h"


Process::Process(std::string procName, int id, int totalInst) 
    : name(procName), 
      processId(id), 
      totalInstructions(totalInst), 
      currentInstruction(0), 
      state(ProcessState::READY) {
}


bool Process::executeStep() {
    if (state == ProcessState::FINISHED) {
        return true;
    }

    state = ProcessState::RUNNING;
    currentInstruction++;

    
    if (currentInstruction >= totalInstructions) {
        state = ProcessState::FINISHED;
        return true;
    }

    return false;
}