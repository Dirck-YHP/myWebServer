#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <semaphore.h>
#include <string>
#include <queue>
#include <mutex>
#include <thread>

#include "../log/Log.h"

class SqlConnPool {
public:
    static SqlConnPool* Instance();

    MYSQL* GetConn();
    void FreeConn(MYSQL* conn);
    int GetFreeConnCount();

    void Init(const char* host, int port, const char* user,
              const char* pwd, const char* dbName, int connSize);
    void ClosePool();

private:
    SqlConnPool() = default;
    ~SqlConnPool() { ClosePool();}

    int MAX_CONN_;

    std::queue<MYSQL*> connQue_;
    std::mutex mtx_;
    sem_t semId_;
};

// RAII的设计，构造函数中获取连接，析构函数中释放连接
// 确保在对象的生命周期结束时及时释放数据库连接，避免资源泄漏
class SqlConnRAII {
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool* connPool) {
        assert(connPool);
        *sql = connPool->GetConn();
        sql_ = *sql;
        connPool_ = connPool;
    }

    ~SqlConnRAII() {
        if (sql_) { connPool_->FreeConn(sql_); }
    }

private:
    MYSQL* sql_;
    SqlConnPool* connPool_;
};

#endif // SQLCONNPOOL_H