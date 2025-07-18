#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t num_threads) : stop(false)
{
    for (size_t i = 0; i < num_threads; ++i)
    {
        workers.emplace_back(&ThreadPool::worker_thread, this);
    }
}
void ThreadPool::worker_thread()
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this]()
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
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace(std::move(task));
    }
    condition.notify_one();
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();

    for (std::thread& worker : workers)
    {
        if (worker.joinable())
            worker.join();
    }
}
