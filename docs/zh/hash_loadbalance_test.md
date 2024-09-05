## 负载均衡hash实现测试

1.拉取git仓库代码，并切换到test分支

```shell
git clone git@github.com:tracer07/trpc-cpp.git
cd trpc-cpp
git checkout test --
```

2.编译配置环境

```shell
bazel build //examples/helloworld/...
```

3.开启多个终端，运行多个server服务，server收到client信息后，会在终端显示

```shell
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber2.yaml
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber3.yaml
```

server的配置文件配置文件位置位于./examples/helloworld/conf/trpc_cpp_fiber.yaml

由于在client的yaml文件配置了端口为11111，22222，33333三个地址，所以需要启动三个终端运行server，每个server对应的port对应为11111，22222，33333.

```yaml
server:
  app: test
  server: helloworld
  admin_port: 8888                    # Start server with admin service which can manage service
  admin_ip: 0.0.0.0
  service:
    - name: trpc.test.helloworld.Greeter
      protocol: trpc                  # Application layer protocol, eg: trpc/http/...
      network: tcp                    # Network type, Support two types: tcp/udp
      ip: 127.0.0.1                     # Service bind ip
      port: 11111                     # Service bind port
```

4.分别存在三个不同caller name 的client，client将每隔5s访问server

```shell
./bazel-bin/examples/helloworld/test/fiber_client --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
./bazel-bin/examples/helloworld/test/fiber_client2 --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
./bazel-bin/examples/helloworld/test/fiber_client3 --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
```

caller name在fiber_client.cc文件，用代码的形式注入

```
fiber_client caller name: trpc.test.helloworld.hello_fiber_client1
fiber_client2 caller name: trpc.test.helloworld.hello_fiber_client2
fiber_client3 caller name: trpc.test.helloworld.hello_fiber_client3
```

client配置文件位于./examples/helloworld/test/conf/trpc_cpp_fiber.yaml

在配置文件中，client的target选项对应第三步启动的三个server

```yaml
client:
  service:
    - name: trpc.test.helloworld.Greeter
      target: 127.0.0.1:11111,127.0.0.1:22222,127.0.0.1:33333      # Fullfill ip:port list here when use `direct` selector.(such as 23.9.0.1:90,34.5.6.7:90)
      protocol: trpc                # Application layer protocol, eg: trpc/http/...
      network: tcp                  # Network type, Support two types: tcp/udp
      selector_name: direct         # Selector plugin, default `direct`, it is used when you want to access via ip:port
      load_balance_name: trpc_consistenthash_load_balance   #支持trpc_consistenthash_load_balance trpc_modulohash_load_balance trpc_polling_load_balance

plugins:
  log:
    default:
      - name: default
        sinks:
          local_file:
            filename: trpc_fiber_client.
  loadbalance:
    trpc_consistenthash_load_balance:
      hash_nodes: 20		#consistent hash中每个实际节点对应的虚拟节点数量
      hash_args: [0]       #支持0-5选项，分别对应selectInfo中的信息.0：caller name 1: client ip 2：client port 3:info.name  4: callee name 5: info.loadbalance name
      hash_func: murmur3  #支持murmur3，city，md5，bkdr，fnv1a
    trpc_modulohash_load_balance:
      hash_args: [0,1]
      hash_fun: city
```


5.运行结果

启动client首先会打印出loadbalance插件update的数据结构，consistenthash打印的是hash ring。

在consistenthash例子中，fiber_client和fiber client2选择了端口为11111的server，而fiber_client3选择了端口为22222的server

```
*****************consistenthash success update****************
hashring info is follow
hash: 25704385743575135  server addresss:127.0.0.1:11111
hash: 70353359594032878  server addresss:127.0.0.1:22222
hash: 279173867688281749  server addresss:127.0.0.1:22222
hash: 375210141783857725  server addresss:127.0.0.1:22222
hash: 406718367139641989  server addresss:127.0.0.1:33333
hash: 467408919241960758  server addresss:127.0.0.1:33333
hash: 682977138908057960  server addresss:127.0.0.1:11111
hash: 738502112444139973  server addresss:127.0.0.1:11111
hash: 961336164618633208  server addresss:127.0.0.1:11111
hash: 1294530552652648092  server addresss:127.0.0.1:11111   
hash: 1339835358552332970  server addresss:127.0.0.1:11111
hash: 1440975125317012970  server addresss:127.0.0.1:22222
hash: 1476327086533475255  server addresss:127.0.0.1:33333
hash: 1778805358949485771  server addresss:127.0.0.1:33333
hash: 2761422071652621985  server addresss:127.0.0.1:22222
hash: 2829729846871718234  server addresss:127.0.0.1:22222
hash: 3085594770821608649  server addresss:127.0.0.1:22222
hash: 3210804927956860464  server addresss:127.0.0.1:11111
hash: 3372271362907740221  server addresss:127.0.0.1:11111
hash: 3484723930157128988  server addresss:127.0.0.1:33333
hash: 5042298646106164944  server addresss:127.0.0.1:11111
hash: 5160425096481595533  server addresss:127.0.0.1:11111
hash: 5677806264232466233  server addresss:127.0.0.1:22222
hash: 6049509532022542492  server addresss:127.0.0.1:22222
hash: 6163921862603218456  server addresss:127.0.0.1:33333
hash: 6811178692204308934  server addresss:127.0.0.1:33333
hash: 7216895431476317498  server addresss:127.0.0.1:22222
hash: 7278084176911851253  server addresss:127.0.0.1:33333
hash: 7309307087249468340  server addresss:127.0.0.1:22222
hash: 7815073739247821846  server addresss:127.0.0.1:22222
hash: 7877725033314940769  server addresss:127.0.0.1:33333
hash: 7878778533766604431  server addresss:127.0.0.1:11111
hash: 8011395986495558754  server addresss:127.0.0.1:22222
hash: 8024998094469030625  server addresss:127.0.0.1:33333
hash: 8229727514660580508  server addresss:127.0.0.1:33333
hash: 8971126116422973855  server addresss:127.0.0.1:22222
hash: 9126347467390732979  server addresss:127.0.0.1:33333
hash: 9669690719668306095  server addresss:127.0.0.1:33333
hash: 9674326317384262916  server addresss:127.0.0.1:11111
hash: 9692645018200744909  server addresss:127.0.0.1:11111
hash: 10700091645353863456  server addresss:127.0.0.1:11111     //例子中的consistenthash 从这里select
hash: 10769524175358196299  server addresss:127.0.0.1:33333
hash: 10856928842092362567  server addresss:127.0.0.1:22222
hash: 11075366471372968531  server addresss:127.0.0.1:11111
hash: 11353796847139991190  server addresss:127.0.0.1:22222
hash: 11666314836972761801  server addresss:127.0.0.1:33333
hash: 12540835281022490548  server addresss:127.0.0.1:33333
hash: 13162856361941185056  server addresss:127.0.0.1:33333
hash: 13280864012700674348  server addresss:127.0.0.1:11111
hash: 13447371717877750318  server addresss:127.0.0.1:33333
hash: 14036416850167660953  server addresss:127.0.0.1:22222
hash: 14109368046851938602  server addresss:127.0.0.1:33333
hash: 14743736294603347528  server addresss:127.0.0.1:22222
hash: 14755750639723125989  server addresss:127.0.0.1:33333
hash: 14760150629320884538  server addresss:127.0.0.1:11111
hash: 14984628613583776616  server addresss:127.0.0.1:11111
hash: 15350104580107007972  server addresss:127.0.0.1:11111
hash: 15504849217779354743  server addresss:127.0.0.1:22222
hash: 15917846871350826359  server addresss:127.0.0.1:22222
hash: 17877771477237698923  server addresss:127.0.0.1:11111
      
//截取部分路由选择信息，比较上面，符合预期的路由选择顺序（插件为consistenthash，client为fiber_client）
consistent hash value is 10649436583591196300
load balance algorithm choose server is 127.0.0.1:11111
get rsp msg: Hello, fiber0
consistent hash value is 10649436583591196300
load balance algorithm choose server is 127.0.0.1:11111
get rsp msg: Hello, fiber1
consistent hash value is 10649436583591196300
load balance algorithm choose server is 127.0.0.1:11111
get rsp msg: Hello, fiber2
consistent hash value is 10649436583591196300
load balance algorithm choose server is 127.0.0.1:11111
get rsp msg: Hello, fiber3
consistent hash value is 10649436583591196300
load balance algorithm choose server is 127.0.0.1:11111
get rsp msg: Hello, fiber4
consistent hash value is 10649436583591196300
load balance algorithm choose server is 127.0.0.1:11111
get rsp msg: Hello, fiber5
consistent hash value is 10649436583591196300
load balance algorithm choose server is 127.0.0.1:11111
get rsp msg: Hello, fiber6
consistent hash value is 10649436583591196300
load balance algorithm choose server is 127.0.0.1:11111
get rsp msg: Hello, fiber7




```

