#ifndef BACKING_STORE_H
#define BACKING_STORE_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Text-file backing store. Every page a process owns is kept here while it is
// not resident in a frame. File format (one line per page):
//
//     <pid> <page> <hex-bytes>\n
//
// e.g.  1 0 00000000000000000000000000000000
//
// The file is rewritten on every change so it can be inspected at any time.
class BackingStore {
public:
    explicit BackingStore(std::size_t frameSize, const std::string& path = "csopesy-backing-store.txt");


    void addProcess(int pid, std::size_t pageCount);


    void removeProcess(int pid);

    // Eviction
    void writeOut(int pid, std::size_t page, const std::vector<std::uint8_t>& data);

    // Fault-in
    void readIn(int pid, std::size_t page, std::vector<std::uint8_t>& data);


    std::size_t pagesPagedIn()  const { return pagesIn_;  }
    std::size_t pagesPagedOut() const { return pagesOut_; }

    void flushToDisk() const;
    const std::string& path() const { return path_; }

private:
    std::size_t frameSize_;
    std::string path_;
    std::size_t pagesIn_ = 0;
    std::size_t pagesOut_ = 0;
    std::unordered_map<int, std::vector<std::vector<std::uint8_t>>> pages_;
};

#endif // BACKING_STORE_H