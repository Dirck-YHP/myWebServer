#include "log.h"

Log::Log() {
    fp_ = nullptr;
    deque_ = nullptr;
    writeThread_ = nullptr;
    lineCount_ = 0;
    toDay_ = 0;
    isAsync_ = false;
}

Log::~Log() {
    while (!deque_->empty()) {
        deque_->flush();        // 唤醒消费者，处理掉剩下的任务，强制置空
    }
}

void Log::flush() {
    if (isAsync_) {
        deque_->flush();    // 只有异步才会用到阻塞队列
    }
    fflush(fp_);        // 清空缓冲区
}