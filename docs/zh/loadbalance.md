# 负载均衡插件使用

## 使用负载均衡插件

要使用负载均衡插件，需在yaml配置文件client:service:load_balance_name 下设置负载均衡插件名字，并在plugins:loadbalance下配置对应插件的相关配置。

以下是一个在yaml配置文件使用负载均衡插件的例子。

```
client:
  service:
    - name: trpc.test.helloworld.Greeter
      target: 127.0.0.1:11111,127.0.0.1:22222,127.0.0.1:33333      # Fullfill ip:port list here when use `direct` selector.(such as 23.9.0.1:90,34.5.6.7:90)
      protocol: trpc                # Application layer protocol, eg: trpc/http/...
      network: tcp                  # Network type, Support two types: tcp/udp
      selector_name: direct         # Selector plugin, default `direct`, it is used when you want to access via ip:port
      load_balance_name: consistent_hash   

plugins:
  log:
    default:
      - name: default
        sinks:
          local_file:
            filename: trpc_fiber_client.
  loadbalance:
    consistent_hash:
      hash_nodes: 20		 
      hash_args: [0]       
      hash_func: murmur3   
```

## 默认负载均衡插件

若没有在client端设置load_balance_name, 默认采用轮询负载均衡插件（trpc_polling_load_balance)，轮询负载均衡插件不需要在plugins下进行配置。

## 负载均衡插件配置

### 哈希负载均衡插件

如果用户在client端设置了hash值，将采用用户提供的hash值来进行路由选择。否则将使用插件的配置来生成hash值（生成hash值的函数见 哈希函数使用 章节）

#### 一致性哈希负载均衡插件（consistent_hash)

以下配置值为默认值，若没对插件进行配置，将采用下面的默认值

```
plugins:
	loadbalance:
		consistent_hash:
          hash_nodes: 160		#consistent hash中每个实际节点对应的虚拟节点数量
          hash_args: [0]       #支持0-5选项，分别对应selectInfo中的信息.0：caller name 1: client ip 2：client port 3:info.name  4: callee name 5: info.loadbalance name
          hash_func: murmur3  #支持murmur3，city，md5，bkdr，fnv1a
```

#### 取模哈希负载均衡插件（modulo_hash)

以下配置值为默认值，若没对插件进行配置，将采用下面的默认值

```
plugins:
	loadbalance:
        modulo_hash:
          hash_args: [0]
          hash_func: murmur3
```

#### 哈希函数使用

在哈希负载均衡插件中使用的哈希函数的定义在文件/trpc/naming/common/util/hash/hash_func.h

文件提供的哈希函数如下：

```
//input为输入的键，hash_func为选择的hash函数，支持“murmur3”，“city”，“md5”，“bkdr”，“fnv1a”
//返回64位哈希值
std::uint64_t Hash(const std::string& input, const std::string& hash_func);

//input为输入的键，hash_func为选择的hash函数，支持“murmur3”，“city”，“md5”，“bkdr”，“fnv1a”，num为模数
//返回64位取模后的哈希值
std::uint64_t Hash(const std::string& input, const std::string& hash_func, uint64_t num);

//input为输入的键，hash_func为选择的hash函数，支持HashFuncName::MD5,HashFuncName::BKDR,HashFuncName::CITY,HashFuncName::BKDR,HashFuncName::MURMUR3,HashFuncName::FNV1A
std::uint64_t Hash(const std::string& input, const HashFuncName& hash_func);

//取模后的哈希值
std::uint64_t Hash(const std::string& input, const HashFuncName& hash_func,uint64_t num);

```

### 其他负载均衡插件
