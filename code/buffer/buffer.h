#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>      // for memset
#include <unistd.h>     // for read, write
#include <sys/uio.h>    // for readv, writev
#include <assert.h>
#include <vector>
#include <string>
#include <atomic>

class Buffer{
public:
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;

    size_t WritableBytes() const;       // 返回可写的字节数
    size_t ReadableBytes() const;        // 返回可读的字节数
    size_t PrependableBytes() const;    // 返回缓冲区中未被写入数据的字节数，即可以在缓冲区头部插入数据的空间大小

    const char* Peek() const;           // 返回读指针
    void EnsureWritable(size_t len);
    void HasWritten(size_t len);

    void Retrieve(size_t len);          // 读指针前移
    void RetrieveUntil(const char* end);    // 读指针前移
    void RetrieveAll();                 // 读指针归零
    std::string RetrieveAllToStr();

    const char* BeginWriteConst() const;    // 返回写指针
    char* BeginWrite();

    void Append(const std::string& str);    // 追加字符串
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);

    ssize_t ReadFd(int fd, int* Errno);
    ssize_t WriteFd(int fd, int* Errno);

private:
    char* BeginPter_();                 // 返回缓冲区的头指针
    const char* BeginPtr_() const;      // 返回缓冲区的头指针
    void MakeSpace_(size_t len);        // 确保有足够的空间

    std::vector<char> buffer_;              // 缓冲区
    std::atomic<std::size_t> readPos_;      // 读的下标
    std::atomic<std::size_t> writePos_;     // 写的下标
};

#endif // BUFFER_H