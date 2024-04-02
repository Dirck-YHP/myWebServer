#ifndef THREAD_POOL
#define THREAD_POOL

#include <assert.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>

class ThreadPool{
public:
    ThreadPool() = default;
    ThreadPool(ThreadPool&&) = default;
    ThreadPool(int numThreads = 8) : isClosed_(false) {
        assert(numThreads > 0);
        for (int i = 0; i < numThreads; i++) {
            threads_.emplace_back([this] {
                while (true) {
                    std::unique_lock<std::mutex> lock(mtx_);
                    condition_.wait(lock, [this] {
                        return !tasks_.empty() || isClosed_;
                    });

                    if (isClosed_ && tasks_.empty()) {
                        return;
                    }

                    std::function<void()> task (std::move(tasks_.front()));
                    tasks_.pop();
                    lock.unlock();
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            isClosed_ = true;
        }

        condition_.notify_all();
        for (auto& t : threads_){
            t.join();
        }
    }

    template<class F, class... Args>
    void AddTask(F&& f, Args&&... args){
        std::function<void()> task = std::bind(std::forward<F>(f), std::forward(args)...);
        
        {
            std::unique_lock<std::mutex> lock(mtx_);
            tasks_.emplace(std::move(task));
        }

        condition_.notify_one();
    }

private:
   std::vector<std::thread> threads_;        // 线程数组
   std::queue<std::function<void()>> tasks_; // 任务队列

   std::mutex mtx_;
   std::condition_variable condition_;

   bool isClosed_; 
};

#endif // THREAD_POOL