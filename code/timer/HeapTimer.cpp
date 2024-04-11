#include "HeepTimer.h"

void HeapTimer::SwapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;      // 结点内部id所在索引位置也要改变
    ref_[heap_[j].id] = j;
}

void HeapTimer::SiftUp_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t parent = (i - 1) / 2;    // 父结点
    while (parent >= 0) {
        if (heap_[parent] > heap_[i]) {
            SwapNode_(parent, i);
            i = parent;
            parent = (i - 1) / 2;
        }
        else 
            break;
    }
}

// 返回值 false：不需要下滑；true：需要下滑
bool HeapTimer::SiftDown_(size_t i, size_t n) {
    assert(i >= 0 && i < heap_.size());
    assert(n >= 0 && n <= heap_.size());    // n: 堆的有效大小
    size_t index = i;
    size_t child = 2 * index + 1;

    while (child < n) {
        // 检查右子节点是否存在且比左子节点的值小，如果是，则选择右子节点作为当前需要比较的子节点
        if (child + 1 < n && heap_[child + 1] < heap_[child]) {
            child++;
        }

        if (heap_[index] > heap_[child]) {
            SwapNode_(index, child);
            index = child;
            child = 2 * index + 1;
        }
        else 
            break;
    }
    return index > i;
}

void HeapTimer::Del_(size_t index) {
    assert(index >= 0 && index < heap_.size());
    size_t tmp = index;
    size_t n = heap_.size() - 1;
    assert(tmp <= n);

    // 将要删除的结点换到最后，然后调整堆
    if (index < n) {
        SwapNode_(tmp, n);
        if (!SiftDown_(tmp, n)) {
            SiftUp_(tmp);
        }
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

// 调整定时器的时间
void HeapTimer::Adjust(int id, int newExpires) {
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = Clock::now() + MS(newExpires);
    SiftDown_(ref_[id], heap_.size());
}

// 添加一个定时器
void HeapTimer::Add(int id, int timeout, const TimeoutCallback& cb) {
    assert(id >= 0);
    if (ref_.count(id)) {
        int tmp = ref_[id];
        heap_[tmp].expires = Clock::now() + MS(timeout);
        heap_[tmp].cb = cb;
        if (!SiftDown_(tmp, heap_.size())) {
            SiftUp_(tmp);
        }
    }
    else {
        size_t n = heap_.size();
        ref_[id] = n;
        heap_.push_back({id, Clock::now() + MS(timeout), cb});  // 添加到堆尾
        SiftUp_(n);
    }
}

void HeapTimer::DoWork(int id) {
    if (heap_.empty() || ref_.count(id) == 0) {
        return;
    }

    size_t i = ref_[id];
    auto node = heap_[i];
    node.cb();      // 触发回调函数
    Del_(i);
}

void HeapTimer::Tick() {
    if (heap_.empty()) return;

    while (!heap_.empty()) {
        TimerNode node = heap_.front();
        // 检查当前时间是否已经超过了定时器事件的触发时间
        // 如果超过了，则退出循环，因为后面的定时器事件都是基于时间递增的，当前事件已经不能触发了
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) 
            break;
        node.cb();
        Pop();
    }
}

void HeapTimer::Pop() {
    assert(!heap_.empty());
    Del_(0);
}

void HeapTimer::Clear() {
    ref_.clear();
    heap_.clear();
}

int HeapTimer::GetNextTick() {
    Tick();
    size_t res = -1;
    if (!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if (res < 0) res = 0;
    }
    return res;
}

