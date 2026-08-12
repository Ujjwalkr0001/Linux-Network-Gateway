#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <iostream>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) : stop_(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this, i]() {
                while (true) {
                    std::function<void()> task;
                    {
                        // Acquire lock protecting task queue
                        std::unique_lock<std::mutex> lock(this->queue_mutex_);

                        // Condition Variable Wait: Sleept until notified AND (task available OR stopping)
                        // Prevents spurious wakeups with predicate lambda
                        this->cv_.wait(lock, [this]() {
                            return this->stop_ || !this->tasks_.empty();
                        });

                        // If stopping and queue is drained, worker thread exits gracefully
                        if (this->stop_ && this->tasks_.empty()) {
                            return;
                        }

                        // Pop next task from queue
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    } // Release mutex lock BEFORE executing task to maximize parallelism!

                    // Execute task outside lock boundary
                    task();
                }
            });
        }
    }

    // Enqueue a task into the thread pool task queue
    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            if (stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            tasks_.emplace([task]() { (*task)(); });
        }
        
        // Notify ONE waiting worker thread that a new task is ready
        cv_.notify_one();
        return res;
    }

    // Destructor: Graceful Shutdown of all worker threads
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        // Wake up ALL worker threads so they notice stop_ = true and exit loops
        cv_.notify_all();

        // Join all worker threads safely
        for (std::thread &worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // Return count of worker threads in pool
    size_t worker_count() const { return workers_.size(); }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_;
};

#endif // THREAD_POOL_HPP
