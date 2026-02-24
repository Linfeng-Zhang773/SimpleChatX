#include "../includes/ThreadPool.hpp"

#include <stdexcept>

ThreadPool::ThreadPool(std::size_t num_threads) : stop(false)
{
    workers.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i)
        workers.emplace_back(&ThreadPool::worker_thread, this);
}

void ThreadPool::worker_thread()
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]
                    { return stop || !tasks.empty(); });
            if (stop && tasks.empty()) return;
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}

void ThreadPool::enqueue(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks.emplace(std::move(task));
    }
    cv.notify_one();
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(mtx);
        stop = true;
    }
    cv.notify_all();
    for (auto& w : workers)
        if (w.joinable()) w.join();
}