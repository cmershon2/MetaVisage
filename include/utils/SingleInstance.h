#ifndef METAVISAGE_UTILS_SINGLEINSTANCE_H
#define METAVISAGE_UTILS_SINGLEINSTANCE_H

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace MetaVisage {

// Named-pipe-based single-instance guard. Call TryAcquire() at startup; if it
// returns false a prior instance is already listening and has been notified
// (the first instance's callback runs on a worker thread). The guard owns a
// listener thread and releases its resources on destruction.
class SingleInstance {
public:
    using RaiseCallback = std::function<void(const std::string& payload)>;

    SingleInstance();
    ~SingleInstance();
    SingleInstance(const SingleInstance&) = delete;
    SingleInstance& operator=(const SingleInstance&) = delete;

    // Tries to become the single instance. If another instance is already
    // running, sends `payload` to it via the named pipe and returns false.
    // On success, starts a listener thread that invokes `onRaise` for each
    // subsequent connection attempt.
    bool TryAcquire(const std::string& payload, RaiseCallback onRaise);

    void Release();

private:
    void ListenLoop();

    void* mutexHandle_ = nullptr; // HANDLE
    std::thread listener_;
    std::atomic<bool> stop_{false};
    RaiseCallback onRaise_;
};

} // namespace MetaVisage

#endif
