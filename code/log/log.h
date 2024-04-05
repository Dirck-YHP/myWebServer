#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>         // vastart va_end
#include <assert.h>
#include <sys/stat.h>       // mkdir
#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log {
public:
    void init(int level, const char* path = "./log",
                const char* suffix = ".log",
                int maxQueueCapacity = 1024);       // 初始化日志实例（阻塞队列最大容量、保存路径、文件后缀）
    
    static Log* Instance();     
    static void FlushLogThread();       // 异步写日志公有方法，调用私有方法AsyncWrite_()
    
    void write(int level, const char* format, ...);       // 将输出内容按照标准格式整理
    void flush();       // 强制刷新缓冲区
    
    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() { return isOpen_;}
    

private:
    Log();
    virtual ~Log();
    void AsyncWrite_();     // 异步写日志方法

private:
    static const int LOG_PATH_LEN = 256;        // 日志文件最长文件名
    static const int LOG_NAME_LEN = 256;        // 日志最长名
    static const int MAX_LINES = 50000;         // 日志文件内的最长日志条数

    const char* path_;          // 路径名
    const char* suffix_;        // 后缀名
    
    int MAX_LINES_;             // 最大日志行数

    int lineCount_;             // 日志行数记录
    int toDay_;                 // 按日期区分文件

    bool isOpen_;

    Buffer buff_;       // 输出的内容，缓冲区
    int level_;         // 日志等级
    bool isAsync_;      // 是否开启异步日志

    FILE* fp_;                                              // 打开log的文件指针
    std::unique_ptr<BlockQueue<std::string>> deque_;        // 阻塞队列
    std::unique_ptr<std::thread> writeThread_;              // 写线程的指针
    std::mutex mtx_;                                        // 同步日志必需的互斥量
};


#endif // LOG_H