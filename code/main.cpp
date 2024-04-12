#include <iostream>
#include "server/WebServer.h"

int main()
{
    WebServer server(
        9006, 3, 60000,     // 端口 ET模式 timeoutMs 
        3306, "root", "yhp20001122..", "my_webserver_db",   // 数据库端口，用户名，密码，数据库名
        12, 8, true, 1, 1024);  // 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量
    server.Start();


    return 0;
}