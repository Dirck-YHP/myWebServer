#include "buffer.h"

Buffer::Buffer(int BufferSize) : 
    buffer_(BufferSize), readPos_(0), writePos_(0) {}

// buffer大小 - 写下标
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

// 写下标 - 读下标
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

// readPos_之前的空间是预备的，可以用来在缓冲区的头部插入新的数据
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

// 返回缓冲区的读指针
const char* Buffer::Peek() const {
    return &buffer_[readPos_];
}

// 确保有足够的空间
void Buffer::EnsureWritable(size_t len) {
    if(len > WritableBytes()) {
        MakeSpace_(len);
    }
    assert(len <= WritableBytes());
}

void Buffer::HasWritten(size_t len) {
    writePos_ += len;
}

void Buffer::Retrieve(size_t len) {
    readPos_ += len;
}

// 读指针前移
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

// 取出所有数据，buffer归零，读写下标归零,在别的函数中会用到
void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    readPos_ = writePos_ = 0;
}

// 将缓冲区中的所有可读数据提取出来并返回为一个字符串
std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

// 返回写指针
const char* Buffer::BeginWriteConst() const {
    return &buffer_[writePos_];
}

char* Buffer::BeginWrite() {
    return &buffer_[writePos_];
}

// 添加str到缓冲区
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWritable(len);    // 确保有足够的空间
    std::copy(str, str + len, BeginWrite());    // 将str拷贝到缓冲区
    HasWritten(len);    // 更新写下标
}

void Buffer::Append(const std::string& str) {
    Append(str.c_str(), str.size());
}

void Buffer::Append(const void* data, size_t len) {
    Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

// 将fd的内容读到缓冲区，即writable的位置
ssize_t Buffer::ReadFd(int fd, int* Errno) {
    char buff[65535];
    struct iovec iov[2];
    size_t writeable = WritableBytes(); // 记录能写多少数据
    // 分散读，保证数据全部读完
    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    ssize_t len = readv(fd, iov, 2);    
    if (len < 0) {
        *Errno = errno;
    } else if (static_cast<size_t>(len) <= writeable) {     // 若len小于writable，说明写区可以容纳len个字节
        writePos_ += len;
    } else {
        writePos_ = buffer_.size();     // 写区写满了,下标移到最后
        Append(buff, static_cast<size_t>(len - writeable));     // 将剩余的数据添加到buffer中
    }
    return len;
}

// 将缓冲区的内容写到fd中
ssize_t Buffer::WriteFd(int fd, int* Errno) {
    ssize_t len = write(fd, Peek(), ReadableBytes());
    if (len < 0) {
        *Errno = errno;
        return len;
    }
    Retrieve(len);    // 读指针前移
    return len;
}

char* Buffer::BeginPtr_() {
    return &buffer_[0];
}

const char* Buffer::BeginPtr_() const{
    return &buffer_[0];
}

// 在进行写入操作之前，确保缓冲区有足够的空间来容纳要写入的数据，
// 并且在数据被丢弃之前尽可能地移动已有的数据以腾出空间
void Buffer::MakeSpace_(size_t len) {
    if (WritableBytes() + PrependableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    }
    else {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readable;
        assert(readable == ReadableBytes());
    }
}