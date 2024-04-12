# 高性能多并发Web服务器（C++11）
## Introduction
用C++实现的高性能WEB服务器
## Function


## Environment
- Ubuntu 18.04
- Modern C++
- MySql
- gits

## Content

|——**bin**              可执行文件
|——**build**            
|————Makefile
|——**code**             源代码
|————buffer
|————http
|————log
|————pool
|————server
|————timer
|————main.cpp
|——**log**              日志文件
|——**resources**        静态资源
|——**webbench-1.5**     压力测试
|——**Makefile**
|——**README.md**


## Build & Usage

**一、配置好自己的数据库**
// 建立yourdb库
create database yourdb;

// 创建user表
USE yourdb;
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;

// 添加数据
INSERT INTO user(username, password) VALUES('name', 'password');


**二、在main.cpp的构造函数初始化中修改数据库配置为你自己定义的**
![](./img/readme_pic1.png)

**三、运行**
make
./bin/server

然后就可以打开浏览器输入ip和端口了，测试的话，可以本地回环，即127.0.0.1:9006  // 9006 这个端口号也需要和main.cpp里面对应

**四、结果**
![](./img/readme_pic2.png.png)

## Thanks

Linux高性能服务器编程，游双著. 

[@qinguoyi](https://github.com/qinguoyi/TinyWebServer)

[@markparticle](https://github.com/markparticle/WebServer)

[@JehanRio](https://github.com/JehanRio/TinyWebServer)
