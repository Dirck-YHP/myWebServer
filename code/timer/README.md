# 简介
&emsp;定义了一个基于小根堆（Min Heap）的定时器类HeapTimer，用于管理一组定时任务。每个定时任务都包含一个ID、一个超时时间戳和一个回调函数。当达到定时任务的超时时间时，相应的回调函数会被执行。
## 详解
传统的定时方案是以固定频率调用起搏函数tick，进而执行定时器上的回调函数。而时间堆的做法则是将所有定时器中超时时间最小的一个定时器的超时值作为心搏间隔，当超时时间到达时，处理超时事件，然后再次从剩余定时器中找出超时时间最小的一个，依次反复即可。

当前系统时间：8:00

1号定时器超时时间：8:05

2号定时器超时时间：8:08

设置心搏间隔：8:05-8:00=5

5分钟到达后处理1号定时器事件，再根据2号超时时间设定心搏间隔.


为了后面处理过期连接的方便，我们给每一个定时器里面放置一个回调函数，用来关闭过期连接。

为了便于定时器结点的比较，主要是后续堆结构的实现方便，我们还需要重载比较运算符。
```c++
struct TimerNode{
public:
    int id;             //用来标记定时器
    TimeStamp expire;   //设置过期时间
    TimeoutCallBack cb; //设置一个回调函数用来方便删除定时器时将对应的HTTP连接关闭
    bool operator<(const TimerNode& t)
    {
        return expire<t.expire;
    }
};
```
添加方法：`timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));`
由于`TimeoutCallBack`的类型是`std::function<void()>`，所以这里采用bind绑定参数。


## 使用方法
1. 添加定时任务：使用add方法添加定时任务，需要提供任务的ID、超时时间（单位通常是毫秒）和一个TimeoutCallBack类型的回调函数，当任务超时时会被调用。
2. 执行定时任务：定期调用tick方法，这会检查堆顶任务是否已超时，并执行相应的回调函数。
3. 查询剩余时间：使用GetNextTick方法获取下一个定时任务的剩余时间（毫秒）。
4. 修改任务超时时间：如果需要调整某个任务的超时时间，使用adjust方法。
5. 手动执行和删除任务：通过doWork方法可以直接执行并删除指定ID的任务。
6. 清除所有任务：调用clear方法可以清除所有定时任务。
7. 实时监控任务状态：可以根据GetNextTick返回的结果动态决定程序的行为，比如决定主线程是否应该休眠等待还是继续其他工作。

## 注意事项
1. 该实现是基于小根堆的，因此在查找和移除最小（即最早到期）的任务时具有O(log N)的时间复杂度。
2. ref_哈希表用于快速查找特定ID的任务，其查找操作具有O(1)的平均时间复杂度。
3. 代码没有处理并发访问的问题，如果需要在多线程环境下使用，需要添加锁或其他同步机制来确保数据一致性。

## 发散
[时间轮和时间堆](https://zhuanlan.zhihu.com/p/472581980)
