#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <cstdint>

#include "instruction.h"

enum class ProcessState {
    READY,
    RUNNING,
    WAITING,
    FINISHED,
    VIOLATED
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

    std::string createdAt;
    std::vector<LogEntry> logs;
    Execution context;

    std::uint64_t nextExecutionTick = 0;


    bool executeStep(int coreId = 0);


    std::size_t memoryRequired = 0;
    std::size_t numPages = 0;
    std::size_t pageFaults = 0;
    std::size_t residentPages = 0;
    std::uint64_t violationAddress = 0;
    std::string violationTime;

     Process(std::string procName, int id, ProgramPtr program, 
            std::size_t memoryBytes, DemandPagingAllocator& allocator);

    

};


#endif
