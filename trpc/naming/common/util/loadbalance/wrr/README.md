## 【腾讯犀牛鸟计划】实现IP直连负载均衡插件 - 加权轮询算法

编译并运行测试程序

```bash
bazel test //trpc/naming/common/util/loadbalance/wrr:wrr_load_balance_test
bazel-bin/trpc/naming/common/util/loadbalance/wrr/wrr_load_balance_test
```

测试结果

```
[root@69bc133c1fb1 trpc]# bazel-bin/trpc/naming/common/util/loadbalance/wrr/wrr_load_balance_test
Running main() from gmock_main.cc
[==========] Running 1 test from 1 test suite.   
[----------] Global test environment set-up.   
[----------] 1 test from WrrLoadBalance
[ RUN      ] WrrLoadBalance.wrr_load_balance_test
host: 114.514.191.981 port: 1003
host: 192.168.000.002 port: 1002
host: 114.514.191.981 port: 1003
host: 114.514.191.981 port: 1003
host: 127.000.000.001 port: 1001
host: 192.168.000.002 port: 1002
host: 114.514.191.981 port: 1003
host: 114.514.191.981 port: 1003
host: 192.168.000.002 port: 1002
host: 114.514.191.981 port: 1003
host: 114.514.191.981 port: 1003
host: 127.000.000.001 port: 1001
host: 192.168.000.002 port: 1002
host: 114.514.191.981 port: 1003

port:1001 count: 2
port:1002 count: 4
port:1003 count: 8
[       OK ] WrrLoadBalance.wrr_load_balance_test (0 ms)
[----------] 1 test from WrrLoadBalance (0 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test suite ran. (0 ms total)
[  PASSED  ] 1 test.
```

测试程序中设置的权重为weights = {1, 2 ,4}，与14个rpc请求的分配到三个服务数量比例（2，4，8）相一致。
