#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Epoller.h"
#include "../timer/HeepTimer.h"
#include "../log/Log.h"
#include "../pool/SqlConnPool.h"
#include "../pool/ThreadPool.h"
#include "../http/HttpConn.h"

class WebServer {
public:
    WebServer(int port, int trigMode, int timeoutMS,
            int sqlPort, const char* sqlUser, const char* sqlPwd, const char* dbName,
            int connPoolNum, int threadNum, bool openLog, int logLevel, int logQueSize);
    ~WebServer();
    void Start();

private:
    bool InitSocket_();                         // 初始化套接字
    void InitEventMode_(int trigMode);          // 初始化事件模式
    void AddClient_(int fd, sockaddr_in addr);  // 添加客户端连接

    void DealListen_();                         // 处理监听事件
    void DealWrite_(HttpConn* client);          // 处理写事件
    void DealRead_(HttpConn* client);           // 处理读事件

    void SendError_(int fd, const char* info);  // 发送错误信息
    void ExtentTime_(HttpConn* client);         // 延长连接时间
    void CloseConn_(HttpConn* client);          // 关闭连接

    void OnRead_(HttpConn* client);             // 读事件处理函数
    void OnWrite_(HttpConn* client);            // 写事件处理函数
    void OnProecess_(HttpConn* client); 

    static const int MAX_FD = 65536;

    static int SetFdNonblock(int fd);           // 设置非阻塞套接字

    int port_;
    bool openLinger_;       // 是否开启连接延迟关闭
    int timeoutMS_;
    bool isClose_;
    int listenFd_;          // 监听套接字描述符
    char* srcDir_;

    uint32_t listenEvent_;      // 监听事件
    uint32_t connEvent_;        // 连接事件

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;   // 用户连接映射
};


#endif // WEBSERVER_H