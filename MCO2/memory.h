#ifndef MEMORY_H
#define MEMORY_H

#include "backing_store.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class InvalidAddressError : public std::runtime_error {
public:
    InvalidAddressError(std::uint64_t addr, const std::string& msg)
        : std::runtime_error(msg), addr_(addr) {}
    std::uint64_t address() const { return addr_; }
private:
    std::uint64_t addr_;
};

// Demand-paging physical memory manager.
//
//  * frames_ is the physical memory (totalFrames = maxOveralMem / frameSize).
//  * Each process owns ceil(memoryBytes / frameSize) virtual pages.
//  * A virtual page maps to a frame only after the first touch (demand paging).
//  * On fault, if no frame is free, the oldest (FIFO) or least-recently-used
//    (LRU) victim is written to the BackingStore and its frame reused.
class DemandPagingAllocator {
public:
    enum class Policy { FIFO, LRU };

    DemandPagingAllocator(BackingStore& store, std::size_t totalMemory, std::size_t frameSize, Policy policy = Policy::FIFO);

    bool createProcess(int pid, std::size_t memoryBytes);
    void destroyProcess(int pid);


    int touchPage(int pid, std::uint64_t vaddr);

    std::uint8_t readByte (int pid, std::uint64_t vaddr);
    void writeByte(int pid, std::uint64_t vaddr, std::uint8_t value);

    bool isResident(int pid, std::uint64_t vaddr) const;


    std::size_t totalMemory() const { return totalFrames_ * frameSize_; }
    std::size_t usedMemory() const { return usedFrames_  * frameSize_; }
    std::size_t freeMemory() const { return totalMemory() - usedMemory(); }
    std::size_t pagesPagedIn() const { return pagesIn_;  }
    std::size_t pagesPagedOut() const { return pagesOut_; }
    std::size_t frameSize()  const { return frameSize_; }
    std::size_t totalFrames()const { return totalFrames_; }
    std::size_t usedFrames() const { return usedFrames_; }

    std::size_t processMemory(int pid) const;
    std::size_t processResidentPages(int pid) const;
    std::size_t processTotalPages(int pid) const;

    std::string visualize() const;

private:
    struct Frame {
        int pid = -1;
        std::size_t vpage = 0;
        std::uint64_t stamp = 0;
        std::vector<std::uint8_t> data;
    };
    struct PageTable {
        std::size_t memoryBytes = 0;
        std::size_t pageCount = 0;
        std::vector<int> frameOfPage; // -1 = page lives only in store
    };

    int findFreeFrame() const;
    int selectVictim() const;
    int faultIn(int pid, std::size_t vpage);

    BackingStore& store_;
    std::size_t frameSize_;
    std::size_t totalFrames_;
    std::size_t usedFrames_ = 0;
    std::size_t pagesIn_ = 0;
    std::size_t pagesOut_ = 0;
    std::uint64_t clock_ = 0;
    Policy policy_;

    std::vector<Frame> frames_;
    std::unordered_map<int, PageTable> tables_;
};

#endif // MEMORY_H