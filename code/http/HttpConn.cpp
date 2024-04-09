#include "HttpConn.h"

const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() {
    fd_ = -1;
    addr_ = {0};
    isClose_ = true;
}

HttpConn::~HttpConn() {
    Close();
}

void HttpConn::init(int sockFd, const sockaddr_in& addr) {
    assert(sockFd > 0);
    userCount++;
    addr_ = addr;
    fd_ = sockFd;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", sockFd, GetIP(), GetPort(), (int)userCount);
}

void HttpConn::Close() {
    response_.UnmapFile();
    if (isClose_ == false) {
        isClose_ = true;
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}

int HttpConn::GetFd() const {
    return fd_;
}

struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const {
    return addr_.sin_port;
}

// 从套接字文件描述符中读取数据到读缓冲区
ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        // 如果发生错误，会将错误码保存在 saveErrno 指向的地址中
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET);     // ET:边沿触发要一次性全部读出
    return len;
}

// 从 iovec 数组中指定的缓冲区依次写入数据到文件描述符 fd_ 中
ssize_t HttpConn::write(int* saveErrno) {
    ssize_t len = -1;
    do {
        // 将 iovec 数组中指定的所有缓冲区的内容依次写入到文件描述符 fd 中
        len = writev(fd_, iov_, iovCnt_);
        if (len < 0) {
            *saveErrno = errno;
            break;
        }
        if (iov_[0].iov_len + iov_[1].iov_len == 0) break;
        else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            // 如果写入的字节数超过了第一个缓冲区的大小
            // 调整第二个缓冲区的位置和大小
            iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            // 如果第一个缓冲区还有剩余数据，则清空写缓冲区
            // 并且将第一个缓冲区的长度设为0
            if (iov_[0].iov_len) {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else {
            // 如果写入的字节数未超过第一个缓冲区的大小
            // 调整第一个缓冲区的位置和大小，并从写缓冲区中移除已写入的数据
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuff_.Retrieve(len);
        }
    }while (isET || ToWriteBytes() > 10240); // 如果启用了 ET 模式，或者待写入的字节数大于 10240，则继续写入
    return len;
}

bool HttpConn::process() {
    request_.Init();
    if (readBuff_.ReadableBytes() <= 0)     // 如果读缓冲区中没有数据可读，返回false
        return false;
    else if (request_.Parse(readBuff_)) {   // 解析HTTP请求
        LOG_DEBUG("%s", request_.Path().c_str());   // 记录请求的路径信息
        // 初始化HttpResponse对象，设置响应报文状态码为200
        response_.Init(srcDir, request_.Path(), request_.IsKeepAlive(), 200);
    }
    else {
        // 如果解析失败，初始化HttpResponse对象，设置响应报文状态码为400
        response_.Init(srcDir, request_.Path(), false, 400);
    }

    response_.MakeResponse(writeBuff_);         // 生成响应报文放入writeBuff_中
    // 设置第一个iovec结构体的基址和长度为写缓冲区中的数据
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;    // 设置iovec数组的元素个数为1


    // 如果响应中包含文件内容且文件长度大于0
    if (response_.FileLen() > 0 && response_.File()) {
        // 设置第二个iovec结构体的基址和长度为文件内容
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;        // 设置iovec数组的元素个数为2
    }

    // 记录文件大小、iovec数组元素个数和待写入字节数
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen(), iovCnt_, ToWriteBytes());
    return true;
}
