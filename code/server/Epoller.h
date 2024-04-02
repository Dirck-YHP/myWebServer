#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>  // epoll_ctl()
#include <unistd.h>     // close()
#include <assert.h>     // assert()
#include <errno.h>
#include <vector>

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024);
    ~Epoller();

    // 向 epoll 实例中添加文件描述符
    bool AddFd(int fd, uint32_t events);

    // 修改 epoll 实例中已添加的文件描述符
    bool ModFd(int fd, uint32_t events);

    // 从 epoll 实例中删除文件描述符
    bool DelFd(int fd);

    // 等待事件的发生，返回就绪的事件数量，timeouts 指定超时时间（单位：毫秒）
    int Wait(int timeoutMs = -1);

    // 获取第 i 个就绪事件的文件描述符
    int GetEventFd(size_t i) const;

    // 获取第 i 个就绪事件的事件类型
    uint32_t GetEvents(size_t i) const;

private:
    int epollFd_; // epoll 实例的文件描述符
    std::vector<struct epoll_event> events_; // 用于存储就绪事件的数组
};

#endif // EPOLLER_H
