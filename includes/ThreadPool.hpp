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
class ThreadPool
{
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();
    void enqueue(std::function<void()> task);
    void worker_thread();
};

#endif