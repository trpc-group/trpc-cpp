### 日志常见问题

### 配置问题

#### 报错日志："DefaultLog instance timecost does not exist"

- 原因： 配置中没有配置 timecost 的 logger 名，但是代码中又用了它，所以报错。
- 解决办法：在配置中添加一个 logger，logger 名叫做 timecost。

```yaml
plugins:
  log:
    default:
      - name: timecost
...
```

#### by_day滚动模式下设置最大保留个数为K, 但是用户看到机器上保留的日志文件个数超过K个

- 原因：日志插件逻辑是如果在K天内重启了，那么，从上次启动开始到本次启动这几天的日志文件一直不会删除。重新启动服务后，框架默认服务保留的日志个数从1开始计算。
- 举例说明：假如保留文件个数9天，用户在第七天重启了机器。那么这七天的日志是不会删除的。 从第七天启动开始, 如果超过10天服务都没有启动的话，那么从第7-17天的日志会按照设置的保留文件个数9进行滚动。但此时因为前7天的文件不会被删除，所以机器上保留的日志文件个数为7+9=16个。
- 解决办法：设置 `plugins->log->default->sinks->remove_timout_file_switch: true` 可删除过时的文件。
- 注意：目前只能删除 by_day 日志，其他待排期。

### 用法问题

#### 框架配置中设置的日志等级为INFO，但实际输出了DEBUG

- 原因：用户自己引用的spdlog的版本覆盖了框架引入的版本。
- 解决办法：统一使用框架的spdlog。

#### 使用TRPC_FMT_INFO格式的宏打印的日志字符串中若有{}，日志会打不出来？

- 原因：这个宏底层使用的是 fmt::format, 而它如果需要支持{}， 就必须对他进行转义。
- 解决办法：对花括号进行转义：{{ 代表左括号， }}代表有括号
- 例子：TRPC_FMT_INFO("123{{}}321"); 将能输出： 123{}321

### 其他问题

#### 服务coredump之后本地日志会有部分缺失

- 原因：框架有个背景线程定时50ms会去把缓存中的数据刷到文件中。假如coredump的时刻正好还没有到下一次刷数据的时候，这时缓存中的日志就会丢失。
- 解决办法：coredump时，建议用 gdb 排查问题，如果要加关键日志，可以使用 `std::cout` 在coredump前刷出。

#### fmt相关编译报错：static assertion failed: Cannot format an argument. To make type T formattable provide a formatter\<T\> specialization

- 原因：C++本身不允许把在enum class当作int类型使用。fmt8.0以上也不再帮助用户从enum class -> int，见 [禁用枚举类的隐式转换int](https://github.com/fmtlib/fmt/issues/1841)。
- 解决办法：用static_cast\<int\>完成类型转换。如果是C++23编译，推荐使用std::to_underlying()。

#### fmt相关编译报错：invalid use of incomplete type 'struct std::variant size\<bool\>'

- 原因：日志打印是否传入了volatile bool 类型，fmt9.0.0以上虽然支持volatile类型，但需要在gcc11以上版本才能编译。volatile 一般在JAVA中使用，C++下建议用户改用atomic。
- 解决办法：把 volatile bool 类型，改为std::atomic\<bool\>。
