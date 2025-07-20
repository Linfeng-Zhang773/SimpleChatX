#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

/// Simple thread pool to run tasks concurrently
class ThreadPool
{
private:
    std::vector<std::thread> workers;        // Worker threads
    std::queue<std::function<void()>> tasks; // Task queue

    std::mutex queue_mutex;            // Protects task queue
    std::condition_variable condition; // Notifies threads
    bool stop;                         // Stop flag for shutdown

public:
    /// Create thread pool with N worker threads
    ThreadPool(size_t num_threads);

    /// Graceful shutdown: join all threads
    ~ThreadPool();

    /// Add a new task to the pool
    void enqueue(std::function<void()> task);

    /// Worker thread function (loops to get tasks)
    void worker_thread();
};

#endif
