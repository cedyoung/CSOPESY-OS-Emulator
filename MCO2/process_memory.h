#ifndef PROCESS_MEMORY_H
#define PROCESS_MEMORY_H

#include "memory.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>

class MemoryAccessViolation : public std::runtime_error {
public:
    MemoryAccessViolation(std::uint64_t addr, const std::string& msg)
        : std::runtime_error(msg), addr_(addr) {}
    std::uint64_t address() const { return addr_; }
private:
    std::uint64_t addr_;
};

// Per-process virtual address space.
//
// Layout (spec):
//   [0, 64)      symbol table, 32 slots x 2 bytes
//   [64, M)      general memory used by READ/WRITE
//   M            = process memory size (from screen -s)
//
// Variable names map to slots 0..31; the value is the uint16 at address slot*2.
// Every access routes through the DemandPagingAllocator, so touching either the
// symbol table or general memory faults the page in on demand.
class ProcessMemory {
public:
    static constexpr std::size_t SYMBOL_TABLE_BYTES = 64;
    static constexpr int MAX_VARIABLES = 32;

    ProcessMemory(DemandPagingAllocator& allocator, int pid, std::size_t memoryBytes);

    bool        declare(const std::string& name, std::uint16_t value);
    bool        hasVar (const std::string& name) const;
    std::uint16_t readVar (const std::string& name);
    bool        writeVar (const std::string& name, std::uint16_t value);

    std::uint16_t readU16 (std::uint64_t addr);        // throws MemoryAccessViolation
    void          writeU16(std::uint64_t addr, std::uint16_t value);

    int         pid()        const { return pid_; }
    std::size_t memorySize() const { return memoryBytes_; }
    std::size_t pageFaults() const { return pageFaults_; }

    std::size_t residentPages() const {
    return allocator_.processResidentPages(pid_);
    }

private:
    int  findOrCreateSlot(const std::string& name);

    DemandPagingAllocator& allocator_;
    int         pid_;
    std::size_t memoryBytes_;
    std::unordered_map<std::string, int> varMap_;
    int         nextFreeSlot_ = 0;
    std::size_t pageFaults_   = 0;

};

#endif // PROCESS_MEMORY_H