#include "ThreadPool.hpp"

/// Constructor: start worker threads
ThreadPool::ThreadPool(size_t num_threads) : stop(false)
{
    for (size_t i = 0; i < num_threads; ++i)
    {
        // Each thread runs the worker loop
        workers.emplace_back(&ThreadPool::worker_thread, this);
    }
}

/// Main loop for worker thread
void ThreadPool::worker_thread()
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            // Wait for new task or stop signal
            condition.wait(lock, [this]()
                           { return stop || !tasks.empty(); });

            // Stop if shutdown and queue is empty
            if (stop && tasks.empty()) return;

            // Get task from queue
            task = std::move(tasks.front());
            tasks.pop();
        }

        // Run the task outside the lock
        task();
    }
}

/// Add a task to the task queue
void ThreadPool::enqueue(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace(std::move(task));
    }

    // Notify one waiting thread
    condition.notify_one();
}

/// Destructor: stop all threads and join
ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }

    // Wake up all waiting threads
    condition.notify_all();

    // Join all threads
    for (std::thread& worker : workers)
    {
        if (worker.joinable())
            worker.join();
    }
}
