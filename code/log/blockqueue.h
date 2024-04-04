#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

/*阻塞队列的实现。
当队列为空时，试图从队列中取出元素的操作会被阻塞，直到队列中有元素可以取出；
当队列已满时，试图向队列中添加元素的操作会被阻塞，直到队列有空间可以容纳新元素*/

#include <assert.h>
#include <deque>
#include <condition_variable>
#include <mutex>

template<typename T>
class BlockQueue {
public:
    explicit BlockQueue(size_t maxsize = 1000);
    ~BlockQueue();

    bool empty();
    bool full();
    void push_back(const T& item);
    void push_front(const T& item);
    bool pop(T& item);                  // 弹出的任务放入item
    bool pop(T& item, int timeout);     // 超时时间
    // void clear();
    T front();
    T back();
    size_t capacity();
    size_t size();

    void flush();
    void Close();

private:
    std::deque<T> deq_;
    std::mutex mtx_;
    bool isClose_;          // 关闭标志
    size_t capacity_;       // 队列容量
    std::condition_variable condConsumer_;  // 消费者条件变量
    std::condition_variable condProducer_;  // 生产者条件变量
};

template<typename T>
BlockQueue<T>::BlockQueue(size_t maxsize) : capacity_(maxsize) {
    assert(maxsize > 0);
    isClose_ = false;
}

template<typename T>
BlockQueue<T>::~BlockQueue() {
    Close();
}

template<typename T>
void BlockQueue<T>::Close() {
    {
        std::lock_guard<std::mutex> locker(mtx_);
        deq_.clear();
        isClose_ = true;
    }
    condProducer_.notify_all();
    condConsumer_.notify_all();
}

template<typename T>
bool BlockQueue<T>::empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

template<typename T>
bool BlockQueue<T>::full() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}

template<typename T>
void BlockQueue<T>::push_back(const T& item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.size() >= capacity_) {
        condProducer_.wait(locker);         // 队列满了，生产者等待消费者唤醒
    }
    deq_.push_back(item);
    condConsumer_.notify_one();             // 向一个等待中的消费者线程发送通知，以告知队列中有新的元素可以被消费了
}

template<typename T>
void BlockQueue<T>::push_front(const T& item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.size() >= capacity_) {
        condProducer_.wait(locker);
    }
    deq_.push_front(item);
    condConsumer_.notify_one();
}

template<typename T>
bool BlockQueue<T>::pop(T& item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.empty()) {
        condConsumer_.wait(locker);         // 队列为空，消费者等待生产者唤醒
        if (isClose_) return false;
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<typename T>
bool BlockQueue<T>::pop(T& item, int timeout) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.empty()) {
        if (condConsumer_.wait_for(locker, std::chrono::seconds(timeout))
                == std::cv_status::timeout) {
                return false;
        }
        if (isClose_) return false;
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<typename T>
T BlockQueue<T>::front() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}

template<typename T>
T BlockQueue<T>::back() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}

template<typename T>
size_t BlockQueue<T>::capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template<typename T>
size_t BlockQueue<T>::size() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

// 唤醒消费者{因为生产者可能会一直等待，所以需要唤醒消费者}
template<typename T>
void BlockQueue<T>::flush() {
    condConsumer_.notify_one();
}


#endif // BLOCKQUEUE_H