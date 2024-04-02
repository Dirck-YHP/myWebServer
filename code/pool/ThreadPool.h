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

                    // 等待，判断是否有新的任务需要执行，如果线程池已经关闭且任务队列为空，则退出线程
                    condition_.wait(lock, [this] {              
                        return !tasks_.empty() || isClosed_;
                    });

                    if (isClosed_ && tasks_.empty()) {
                        return;
                    }

                    std::function<void()> task (std::move(tasks_.front()));
                    tasks_.pop();
                    lock.unlock();  // 任务已经取出来了，所以提前解锁，让其他线程可以继续添加任务
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

        // 通知所有等待的线程线程池已经关闭,join()等待所有线程结束。
        condition_.notify_all();
        for (auto& t : threads_){
            t.join();
        }
    }

    template<class F, class... Args>
    void AddTask(F&& f, Args&&... args){
        // 将函数f和其参数args...绑定到task上
        std::function<void()> task = std::bind(std::forward<F>(f), std::forward(args)...);
        
        {
            std::unique_lock<std::mutex> lock(mtx_);
            tasks_.emplace(std::move(task));        // 添加到任务队列中
        }

        condition_.notify_one();        // 通知等待的线程有新任务可以执行，唤醒一个程序
    }

private:
   std::vector<std::thread> threads_;        // 线程数组
   std::queue<std::function<void()>> tasks_; // 任务队列

   std::mutex mtx_;
   std::condition_variable condition_;

   bool isClosed_; 
};

#endif // THREAD_POOL