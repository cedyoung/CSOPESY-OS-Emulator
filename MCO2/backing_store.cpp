#include "backing_store.h"
#include <fstream>
#include <iomanip>

BackingStore::BackingStore(std::size_t frameSize, const std::string& path)
    : frameSize_(frameSize), path_(path) {
    std::ofstream(path_, std::ios::trunc); // create / clear
}

void BackingStore::addProcess(int pid, std::size_t pageCount) {
    pages_[pid].assign(pageCount, std::vector<std::uint8_t>(frameSize_, 0));
    flushToDisk();
}

void BackingStore::removeProcess(int pid) {
    pages_.erase(pid);
    flushToDisk();
}

void BackingStore::writeOut(int pid, std::size_t page, const std::vector<std::uint8_t>& data) {
    auto& pages = pages_[pid];
    if (page >= pages.size())
        pages.resize(page + 1, std::vector<std::uint8_t>(frameSize_, 0));
    pages[page] = data;
    ++pagesOut_;
    flushToDisk();
}

void BackingStore::readIn(int pid, std::size_t page, std::vector<std::uint8_t>& data) {
    auto& pages = pages_[pid];
    if (page >= pages.size())
        pages.resize(page + 1, std::vector<std::uint8_t>(frameSize_, 0));
    data = pages[page];
    ++pagesIn_;
}

void BackingStore::flushToDisk() const {
    std::ofstream f(path_, std::ios::trunc);
    if (!f) return;
    for (const auto& [pid, pages] : pages_) {
        for (std::size_t i = 0; i < pages.size(); ++i) {
            f << pid << ' ' << i << ' ';
            for (std::uint8_t b : pages[i])
                f << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(b);
            f << '\n';
        }
    }
}