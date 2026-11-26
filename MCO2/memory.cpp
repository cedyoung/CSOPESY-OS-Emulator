#include "memory.h"
#include <limits>
#include <sstream>

DemandPagingAllocator::DemandPagingAllocator(BackingStore& store,
                                             std::size_t totalMemory,
                                             std::size_t frameSize,
                                             Policy policy)
    : store_(store),
      frameSize_(frameSize),
      totalFrames_(totalMemory / frameSize),
      policy_(policy) {
    if (frameSize == 0 || totalFrames_ == 0)
        throw std::invalid_argument("DemandPagingAllocator: invalid sizes");
    frames_.resize(totalFrames_);
    for (auto& f : frames_) f.data.assign(frameSize_, 0);
}

bool DemandPagingAllocator::createProcess(int pid, std::size_t memoryBytes) {
    if (memoryBytes == 0 || tables_.count(pid)) return false;

    PageTable pt;
    pt.memoryBytes = memoryBytes;
    pt.pageCount   = (memoryBytes + frameSize_ - 1) / frameSize_;
    pt.frameOfPage.assign(pt.pageCount, -1); // all pages swapped out
    tables_.emplace(pid, std::move(pt));

    store_.addProcess(pid, tables_[pid].pageCount);
    return true;
}

void DemandPagingAllocator::destroyProcess(int pid) {
    auto it = tables_.find(pid);
    if (it == tables_.end()) return;

    for (int frame : it->second.frameOfPage) {
        if (frame >= 0) {
            frames_[frame] = Frame{};
            frames_[frame].data.assign(frameSize_, 0);
            --usedFrames_;
        }
    }
    tables_.erase(it);
    store_.removeProcess(pid);
}

int DemandPagingAllocator::faultIn(int pid, std::size_t vpage) {
    int frame = findFreeFrame();
    if (frame == -1) {
        frame = selectVictim(); // eviction branch
        if (frame == -1) return -1;
        Frame& victim = frames_[frame];
        store_.writeOut(victim.pid, victim.vpage, victim.data);
        ++pagesOut_;
        auto vit = tables_.find(victim.pid);
        if (vit != tables_.end())
            vit->second.frameOfPage[victim.vpage] = -1;
        // reuse frame: usedFrames_ unchanged
    } else {
        ++usedFrames_;
    }

    Frame& f = frames_[frame];
    f.pid   = pid;
    f.vpage = vpage;
    f.stamp = ++clock_;
    store_.readIn(pid, vpage, f.data); // load from backing store
    ++pagesIn_;

    tables_[pid].frameOfPage[vpage] = frame;
    return frame;
}

int DemandPagingAllocator::touchPage(int pid, std::uint64_t vaddr) {
    auto it = tables_.find(pid);
    if (it == tables_.end())
        throw InvalidAddressError(vaddr, "process not registered");
    if (vaddr >= it->second.memoryBytes)
        throw InvalidAddressError(vaddr, "address out of bounds");

    std::size_t vpage = vaddr / frameSize_;
    int& slot = it->second.frameOfPage[vpage];

    if (slot != -1) { // already resident
        if (policy_ == Policy::LRU) frames_[slot].stamp = ++clock_;
        return slot;
    }
    return faultIn(pid, vpage); // page fault
}

std::uint8_t DemandPagingAllocator::readByte(int pid, std::uint64_t vaddr) {
    int frame = touchPage(pid, vaddr);
    return frames_[frame].data[vaddr % frameSize_];
}

void DemandPagingAllocator::writeByte(int pid, std::uint64_t vaddr,
                                      std::uint8_t value) {
    int frame = touchPage(pid, vaddr);
    frames_[frame].data[vaddr % frameSize_] = value;
}

bool DemandPagingAllocator::isResident(int pid, std::uint64_t vaddr) const {
    auto it = tables_.find(pid);
    if (it == tables_.end()) return false;
    if (vaddr >= it->second.memoryBytes) return false;
    return it->second.frameOfPage[vaddr / frameSize_] != -1;
}

int DemandPagingAllocator::findFreeFrame() const {
    for (std::size_t i = 0; i < frames_.size(); ++i)
        if (frames_[i].pid == -1) return static_cast<int>(i);
    return -1;
}

int DemandPagingAllocator::selectVictim() const {
    int victim = -1;
    std::uint64_t oldest = std::numeric_limits<std::uint64_t>::max();
    for (std::size_t i = 0; i < frames_.size(); ++i) {
        if (frames_[i].pid != -1 && frames_[i].stamp < oldest) {
            oldest = frames_[i].stamp;
            victim = static_cast<int>(i);
        }
    }
    return victim;
}

std::size_t DemandPagingAllocator::processMemory(int pid) const {
    auto it = tables_.find(pid);
    return it == tables_.end() ? 0 : it->second.memoryBytes;
}

std::size_t DemandPagingAllocator::processResidentPages(int pid) const {
    auto it = tables_.find(pid);
    if (it == tables_.end()) return 0;
    std::size_t n = 0;
    for (int f : it->second.frameOfPage) if (f != -1) ++n;
    return n;
}

std::size_t DemandPagingAllocator::processTotalPages(int pid) const {
    auto it = tables_.find(pid);
    return it == tables_.end() ? 0 : it->second.pageCount;
}

std::string DemandPagingAllocator::visualize() const {
    std::ostringstream out;
    out << "Frames: " << usedFrames_ << '/' << totalFrames_
        << " (page size " << frameSize_ << " B)\n";
    for (std::size_t i = 0; i < frames_.size(); ++i) {
        const Frame& f = frames_[i];
        if (f.pid == -1) out << "  [" << i << "] free\n";
        else             out << "  [" << i << "] pid=" << f.pid
                             << " vpage=" << f.vpage << '\n';
    }
    return out.str();
}