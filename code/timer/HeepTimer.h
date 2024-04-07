#ifndef HEAPTIMER_H
#define HEAPTIMER_H

#include <time.h>
#include <assert.h>
#include <arpa/inet.h>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <chrono>

#include "../log/Log.h"

typedef std::function<void()> TimeoutCallback;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;
    TimeStamp expires;      // 定时器生效的绝对时间
    TimeoutCallback cb;     // 超时回调函数
    
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }

    bool operator>(const TimerNode& t) {
        return expires > t.expires;
    }
};

class HeapTimer {
public:
    HeapTimer() { heap_.reserve(64);}
    ~HeapTimer() { Clear(); }

    void Adjust(int id, int newExpires);
    void Add(int id, int timeout, const TimeoutCallback& cb);
    void DoWork(int id);
    void Clear();
    void Tick();
    void Pop();
    int GetNextTick();

private:
    void Del_(size_t i);
    void SiftUp_(size_t i);
    bool SiftDown_(size_t i, size_t n);
    void SwapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;    // 堆
    std::unordered_map<int, size_t> ref_;    // id对应的在heap_中的下标，方便用heap_下标快速定位到id
};


#endif // HEAPTIMER_H