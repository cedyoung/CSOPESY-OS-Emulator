#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>

#include "instruction.h"

enum class ProcessState {
    READY,
    RUNNING,
    WAITING,
    FINISHED
};

struct LogEntry {
    std::string timestamp;
    int coreId;
    std::string message;
};

struct Process {
    std::string name;
    int processId;
    int totalInstructions;
    int currentInstruction;
    ProcessState state;

     
    std::vector<LogEntry> logs;
    Execution context;

    Process(std::string procName, int id, ProgramPtr program);
    bool executeStep(int coreId = 0);


};

#endif
