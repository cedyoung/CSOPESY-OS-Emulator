#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>

enum class ProcessState {
    READY,
    RUNNING,
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

    Process(std::string procName, int id, int totalInst);
    bool executeStep(int coreId = 0);


};

#endif