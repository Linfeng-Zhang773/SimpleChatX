#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

/**
 * @brief A simple thread pool that dispatches std::function<void()> tasks.
 *
 * Workers block on a condition variable until a task is available or
 * the pool is shut down.
 */
class ThreadPool
{
public:
    /**
     * @brief Construct and launch @p num_threads worker threads.
     * @param num_threads Number of threads in the pool.
     */
    explicit ThreadPool(std::size_t num_threads);

    /// @brief Signal stop, drain the queue, and join all workers.
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief Submit a task for asynchronous execution.
     * @param task Callable to run on a worker thread.
     * @throws std::runtime_error if the pool has been stopped.
     */
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop;

    /// @brief Worker loop: dequeue and execute tasks until stopped.
    void worker_thread();
};