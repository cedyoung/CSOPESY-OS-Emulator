#ifndef PROCESS_H
#define PROCESS_H

#include <string>


enum class ProcessState {
    READY,
    RUNNING,
    FINISHED
};

struct Process {
    std::string name;
    int processId;
    int totalInstructions;
    int currentInstruction;
    ProcessState state;

     
    Process(std::string procName, int id, int totalInst);

    bool executeStep();
};

#endif