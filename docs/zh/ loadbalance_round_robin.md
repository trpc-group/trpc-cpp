# 一、平滑加权轮询负载均衡插件

为了避免传统加权轮询中可能出现的负载不均衡问题，采用平滑加权轮询（Smooth Weighted Round-Robin，简称 SWRR）。该方法通常用于将请求分配到一组服务器中，以实现更均衡的负载分配。它在传统的加权轮询（Weighted Round-Robin）基础上，通过平滑处理来避免负载分配的不均匀性。

# 二、具体实现

平滑加权轮询算法的目标是减少传统加权轮询算法中的不均衡现象，尤其是在请求到达不均匀或服务器负载变化的情况下。该算法通过以下方式来平滑负载分配：

- **加权池**：服务器的权重决定了它们在加权池中的出现频率。平滑加权轮询算法会在轮询过程中根据权重动态调整每台服务器的“虚拟”权重，使得负载分配更均匀。
- **平滑处理**：算法会计算每台服务器的负载和权重，并在每次分配请求时，基于服务器的负载情况调整请求的分配。这种方式确保高负载的服务器不会被过度分配请求，从而避免负载过度集中。
- **动态调整**：当服务器的负载发生变化时，算法会动态调整权重值，以确保负载在服务器之间的分配更加合理。例如，如果某台服务器变得很繁忙，它的权重值可能会被降低，从而减少它接收请求的频率。

### 示例

假设有三个节点 A、B、C，它们的权重分别为 5、1、1。初始时，各节点的当前权重为 0。

- **第一次请求：**
  - A：0 + 5 = 5
  - B：0 + 1 = 1
  - C：0 + 1 = 1
  - 选择 A，因为 A 的当前权重最高。然后 A 的当前权重变为 5 - (5 + 1 + 1) = -2。

- **第二次请求：**
  - A：-2 + 5 = 3
  - B：1 + 1 = 2
  - C：1 + 1 = 2
  - 选择 A，因为 A 的当前权重仍然最高。然后 A 的当前权重变为 3 - (5 + 1 + 1) = -4。

- **第三次请求：**
  - A：-4 + 5 = 1
  - B：2 + 1 = 3
  - C：2 + 1 = 3
  - 选择 B（或 C），因为 B 和 C 的当前权重相同且最高。然后 B 的当前权重变为 3 - (5 + 1 + 1) = -4。

通过这个过程，可以看到虽然 A 的权重最高，但它不会在每次轮询中都被选中。随着轮询次数的增加，各节点的选择机会逐渐接近其权重比例，实现平滑的负载均衡。

# 三、使用方法

在客户端配置文件中，例如 `trpc_cpp_fiber.yaml`，在 `target` 配置中使用带权重的 `ip:port:weight` 格式来指定端点的方案：

```yaml
client:
  service:
    - name: trpc.test.helloworld.Greeter
      target: 127.0.0.1:10000:1,127.0.0.1:20000:2,127.0.0.1:30000:3      # Fullfill ip:port list here when use `direct` selector.(such as 23.9.0.1:90:1,34.5.6.7:90:2)
      protocol: trpc                # Application layer protocol, eg: trpc/http/...
      network: tcp                  # Network type, Support two types: tcp/udp
      selector_name: direct         # Selector plugin, default `direct`, it is used when you want to access via ip:port
      load_balance_name: trpc_smooth_weighted_polling_load_balance
```

在客户端文件中，注册负载均衡插件，使用 `::trpc::loadbalance::Init()` 注册插件：

```cpp
int Run() {
  ::trpc::loadbalance::Init();
  auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>(FLAGS_service_name);
  return 0;
}

int main(int argc, char* argv[]) {
  ParseClientConfig(argc, argv);
  // If the business code is running in trpc pure client mode,
  // the business code needs to be running in the `RunInTrpcRuntime` function
  return ::trpc::RunInTrpcRuntime([]() { return Run(); });
}
```
可以参考下面文档获取更多信息https://docs.qq.com/doc/DTHdBVUxybHV2ekFH