#include "process_memory.h"

ProcessMemory::ProcessMemory(DemandPagingAllocator& allocator, int pid,
                             std::size_t memoryBytes)
    : allocator_(allocator), pid_(pid), memoryBytes_(memoryBytes) {}

int ProcessMemory::findOrCreateSlot(const std::string& name) {
    auto it = varMap_.find(name);
    if (it != varMap_.end()) return it->second;
    if (nextFreeSlot_ >= MAX_VARIABLES) return -1;
    int slot = nextFreeSlot_++;
    varMap_[name] = slot;
    return slot;
}

bool ProcessMemory::declare(const std::string& name, std::uint16_t value) {
    int slot = findOrCreateSlot(name);
    if (slot < 0) return false;
    writeU16(static_cast<std::uint64_t>(slot) * 2, value);
    return true;
}

bool ProcessMemory::hasVar(const std::string& name) const {
    return varMap_.count(name) > 0;
}

std::uint16_t ProcessMemory::readVar(const std::string& name) {
    int slot = findOrCreateSlot(name);
    if (slot < 0) return 0;
    return readU16(static_cast<std::uint64_t>(slot) * 2);
}

bool ProcessMemory::writeVar(const std::string& name, std::uint16_t value) {
    int slot = findOrCreateSlot(name);
    if (slot < 0) return false;
    writeU16(static_cast<std::uint64_t>(slot) * 2, value);
    return true;
}


std::uint16_t ProcessMemory::readU16(std::uint64_t addr) {
    if (addr + 2 > memoryBytes_)
        throw MemoryAccessViolation(addr, "read out of bounds");
    std::uint16_t out = 0;
    for (int i = 0; i < 2; ++i) {
        std::uint64_t a = addr + i;
        bool wasIn = allocator_.isResident(pid_, a);
        std::uint8_t b = allocator_.readByte(pid_, a);
        if (!wasIn) ++pageFaults_;
        out |= static_cast<std::uint16_t>(b) << (i * 8);
    }
    return out;
}

void ProcessMemory::writeU16(std::uint64_t addr, std::uint16_t value) {
    if (addr + 2 > memoryBytes_)
        throw MemoryAccessViolation(addr, "write out of bounds");
    for (int i = 0; i < 2; ++i) {
        std::uint64_t a = addr + i;
        bool wasIn = allocator_.isResident(pid_, a);
        allocator_.writeByte(pid_, a, (value >> (i * 8)) & 0xFF);
        if (!wasIn) ++pageFaults_;
    }
}