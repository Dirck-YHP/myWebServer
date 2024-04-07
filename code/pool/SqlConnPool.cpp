#include "SqlConnPool.h"

SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(const char* host, int port, const char* user,
                       const char* pwd, const char* dbName, int connSize) {
    assert(connSize > 0);
    for (int i = 0; i < connSize; ++i) {
        MYSQL* conn = nullptr;
        conn = mysql_init(conn);

        if (!conn) {
            LOG_ERROR("mysql_init error!");
            assert(conn);
        }

        conn = mysql_real_connect(conn, host, user, pwd, dbName, port, nullptr, 0);
        if (!conn) {
            LOG_ERROR("mysql_real_connect error!");
        }
        connQue_.emplace(conn);
    }
    MAX_CONN_ = connSize;
    sem_init(&semId_, 0, MAX_CONN_);
}

MYSQL* SqlConnPool::GetConn() {
    MYSQL* conn = nullptr;
    if (connQue_.empty()) {
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    sem_wait(&semId_);
    {
        std::lock_guard<std::mutex> locker(mtx_);
        conn = connQue_.front();
        connQue_.pop();
    }
    return conn;
}

// 将数据库连接归还到连接池
void SqlConnPool::FreeConn(MYSQL* conn) {
    assert(conn);
    std::lock_guard<std::mutex> locker(mtx_);
    connQue_.emplace(conn);
    sem_post(&semId_);
}

void SqlConnPool::ClosePool() {
    std::lock_guard<std::mutex> locker(mtx_);
    while (!connQue_.empty()) {
        auto conn = connQue_.front();
        connQue_.pop();
        mysql_close(conn);
    }
    mysql_library_end();        // 释放整个 MySQL 客户端库的资源
}

int SqlConnPool::GetFreeConnCount() {
    std::lock_guard<std::mutex> locker(mtx_);
    return connQue_.size();
}