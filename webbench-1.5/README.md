## webbench 的原理
⾸先在主进程中 fork 出多个⼦进程，每个⼦进程都循环做 web 访问测试。
⼦进程把访问的结果通过 pipe 告诉⽗进程，⽗进程做最终的统计结果。
webbench 最多可以模拟3万个并发连接去测试⽹站的负载能⼒。

