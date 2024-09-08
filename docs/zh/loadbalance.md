## 负载均衡插件使用

#### 1、使用负载均衡插件

要使用负载均衡插件，需在client端的yaml配置文件设置load_balance_name并在plugins下设置loadbalance配置。

以下以一致性哈希负载均衡配置为例。

```
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
```

#### 2、负载均衡默认配置

若没有在client端设置load_balance_name, 默认采用轮询负载均衡插件（trpc_polling_load_balance)，轮询负载均衡插件无配置选项。

#### 3、哈希负载均衡插件配置

一致性哈希负载均衡插件（trpc_consistenthash_load_balance)

以下配置值为默认值，若没对插件进行配置，将采用下面的默认值

```
plugins:
	trpc_consistenthash_load_balance:
      hash_nodes: 160		#consistent hash中每个实际节点对应的虚拟节点数量
      hash_args: [0]       #支持0-5选项，分别对应selectInfo中的信息.0：caller name 1: client ip 2：client port 3:info.name  4: callee name 5: info.loadbalance name
      hash_func: murmur3  #支持murmur3，city，md5，bkdr，fnv1a
```

取模哈希负载均衡插件（trpc_modulohash_load_balance)

以下配置值为默认值，若没对插件进行配置，将采用下面的默认值

```
plugins:
	trpc_modulohash_load_balance:
      hash_args: [0]
      hash_fun: murmur3
```

#### 4、哈希函数使用

哈希函数的定义在文件/trpc/naming/common/util/hash/hash_func.h

文件提供的哈希函数如下：

```
//input为输入的键，hash_func为选择的hash函数，支持“murmur3”，“city”，“md5”，“bkdr”，“fnv1a”
//返回64位哈希值
std::uint64_t Hash(const std::string& input, const std::string& hash_func);

//input为输入的键，hash_func为选择的hash函数，支持“murmur3”，“city”，“md5”，“bkdr”，“fnv1a”，num为模数
//返回64位取模后的哈希值
std::uint64_t Hash(const std::string& input, const std::string& hash_func, uint64_t num);

//input为输入的键，hash_func为选择的hash函数，支持HashFuncName::MD5,HashFuncName::BKDR,HashFuncName::CITY,HashFuncName::BKDR,HashFuncName: