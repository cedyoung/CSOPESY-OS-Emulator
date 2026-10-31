#include "scheduler.h"

#include <chrono>

FCFSScheduler::FCFSScheduler() = default;

FCFSScheduler::~FCFSScheduler() {
    stop();
}

void FCFSScheduler::initialize(const Config& newConfig) {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    if (running) {
        return;
    }

    config = newConfig;
    initialized = true;
}

void FCFSScheduler::start() {
    {
        std::lock_guard<std::mutex> lock(schedulerMutex);

        if (!initialized || running) {
            return;
        }

        running = true;
    }

    workerThreads.clear();

    for (int core = 0; core < config.numCpu; ++core) {
        workerThreads.emplace_back(
            &FCFSScheduler::workerLoop,
            this,
            core
        );
    }

    schedulerThread = std::thread(
        &FCFSScheduler::schedulerLoop,
        this
    );
}

void FCFSScheduler::stop() {
    {
        std::lock_guard<std::mutex> lock(schedulerMutex);

        if (!running) {
            return;
        }

        running = false;
    }

    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }

    for (std::thread& worker : workerThreads) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workerThreads.clear();
}

void FCFSScheduler::schedulerLoop() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(schedulerMutex);

            if (!running) {
                return;
            }
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(5)
        );
    }
}

void FCFSScheduler::workerLoop(int coreId) {
    (void)coreId;

    while (true) {
        {
            std::lock_guard<std::mutex> lock(schedulerMutex);

            if (!running) {
                return;
            }
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(5)
        );
    }
}
