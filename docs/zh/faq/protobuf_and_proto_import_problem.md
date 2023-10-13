### protobuf/proto文件引入常见问题

#### trpc_proto_library编译失败

- 原因：常见于pb文件使用bazel内置规则 `proto_library` 或 `cc_proto_library` 定义，但却把这些规则放在了 `trpc_proto_library` 的deps里导致出错。
- 解决办法：
  - 最好的办法是，所有proto文件对应的bazel规则统一通过 `trpc_proto_library` 定义。
  - 如果确实有只能通过bazel内置规则定义的，请放在 `trpc_proto_library` 的 `native_proto_deps` 或者 `native_cc_proto_deps` 列表里。

#### import pb文件使用时出现类型未定义的问题

- 原因：在import之后，如果 `message` 定义不在同一 `package` 下，会报变量未定义的问题。
- 解决办法：
  1. 统一 `package` 命名。
  2. import后使用类型时加上 `.` 分隔的 `package 前缀`。

#### 如何引入 any.proto 等 protobuf 库里的文件

- 场景：protobuf库里，定义了很多有用的proto文件，比如：any.proto 允许你定义任意类型的字段。
- 解决办法：引入 any.proto 对应的bazel规则即可

```python
trpc_proto_library(
  name = "my_car_trpc_proto",
  # my_car.proto 里 import "google/protobuf/any.proto"
  srcs = ["my_car.proto"],
 ...
 native_proto_deps = [
  "@com_google_protobuf//:any_proto",
 ]
 ...
)
```

#### trpc input pb response parese from array failed

- 原因：message消息体类型为string的字段，赋值了非UTF-8字段，protobuf库会在序列化/反序列化时会出错。
- 场景：常见于数据库里表字段使用了非UTF-8编码，从数据库里读出值后直接赋值给protobuf的message消息体的string类型成员。
- 解决办法：对这种场景，应该使用bytes类型，或者业务逻辑里完成到UTF-8编码的转换。
