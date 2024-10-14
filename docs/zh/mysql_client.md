

# 背景

MySQL数据库已经成为构建互联网世界的基石，尤其是其支持事务的特性，支撑了很多互联网的很多应用场景，比如电商、支付等。我们在tRPC-cpp中提供了以客户端的方式访问下游MySQL服务。

该组件基于MySQL 8.0的c api(libmysqlclient)，对MySQL server进行调用。并使用框架内的线程进行驱动，因此对api的调用不会阻塞业务线程和fiber协程。以下相关类均在 **namespace trpc::mysql** 之内。

#  原理

##  查询结果类

```c++
template <typename... Args>
class MysqlResults
```

`MysqlResults` 是一个模板类，用于存储 MySQL 查询结果。它支持多种模式来处理查询结果，包括：

- **OnlyExec**: 无结果集的执行模式
- **NativeString**:  `std::string_view` 数据
- **IterMode**: 迭代器遍历原始数据
- **BindType**: 将查询结果绑定到特定类型。

### 结果集类型

`MysqlResults` 使用 `std::vector` 保存结果集（除了 `OnlyExec`），每一个元素为一行，其类型与模板参数有关

| 模板参数       | 结果集类型                                   |
| -------------- | -------------------------------------------- |
| `NativeString` | `std::vector<std::vector<std::string_view>>` |
| `IterMode`     | `std::vector<MysqlRow>`                      |
| `Args...`      | `std::vector<std::tuple<Args...>>`           |


### 通用函数成员

|                                                    | **描述**                                                     |
| -------------------------------------------------- | ------------------------------------------------------------ |
| `bool IsSuccess()`                                 | 查询是否成功。                                               |
| `const std::string& GetErrorMessage()`             | 如果查询有错误，返回错误原因，否则为空。                     |
| `auto& GetResultSet()`                             | 返回结果集引用（除 `OnlyExec`），结果集是一个 `std::vector`，每一个元素代表一行，其具体类型和模板参数有关。 |
| `bool GetResultSet(T& res)`                        | 将结果集移动到到参数 `res` 中，参数类型需要和**结果集类型**相同。只能调用一次 |
| `std::vector<std::vector<uint8_t>>& GetNullFlag()` | 每一个元素表示查询结果中对应的值是否为空。                   |



### 使用示例

```
/*
mysql> select * from users;
+----+----------+-------------------+---------------------+------------+
| id | username | email             | created_at          | meta       |
+----+----------+-------------------+---------------------+------------+
|  1 | alice    | alice@example.com | 2024-09-08 13:16:24 | NULL       |
|  2 | bob      | bob@abc.com       | 2024-09-08 13:16:24 | NULL       |
|  3 | carol    | carol@example.com | 2024-09-08 13:16:24 | NULL       |
|  4 | rose     | NULL              | 2024-09-08 13:16:53 | NULL       |
+----+----------+-------------------+---------------------+------------+
*/
```



#### class MysqlResults\<OnlyExec\>

```c++
class MysqlResults<OnlyExec>
    
// example
MysqlResults<OnlyExec> exec_res;
proxy->Execute(client_context, exec_res,
             "insert into users (username)"
             "values (?)", "Jack");
if(exe_res.IsSuccess())
	size_t n_rows = exec_res.GetAffectedRows()
else
    std::string error = exec_res.GetErrorMessage()
```

用于像 `INSERT`、`UPDATE` 这类不会返回结果集的查询操作。通过 `GetAffectedRows()` 获取影响的行。



#### class MysqlResults\<NativeString\>

```c++
class MysqlResult<NativeString>

MysqlResults<NativeString> query_res;
proxy->Execute(client_context, query_res,
             "select * from users");

using ResSetType = std::vector<std::vector<std::string_view>>;
// ResSetType& res_data = res.GetResultSet();
auto& res_data = res.GetResultSet();

std::cout << res_data[0][1] << std::endl;  // alice@example.com
if(res.GetNullFlag()[3][2] != 0)
    std::cout << "rose's email is null" << std::endl;

```

如果模板参数指定为 `NativeString` ，则结果集会以 `std::vector<std::vector<std::string_view>>` 返回，每一个 `std::vector<std::string_view>` 表示一行，里面每一个元素是该行的对应字段值。其中 `std::string_view` 对应的内存资源在 MysqlResults析构后会被释放。



#### class MysqlResults\<IterMode\>

```c++
class MysqlResult<IterMode>

MysqlResults<IterMode> query_res;
proxy->Execute(client_context, query_res,
             "select * from users");

for (auto row : res) {
    // type row: MysqlRow
    for (unsigned int i = 0; i < row.NumFields(); i++) 
        if(!row.IsFieldNull[i])
        	std::cout << row.GetFieldData(i) << ", ";
    std::cout << std::endl;
}

for (auto row : res) {
    for (auto field_itr = row.begin(); field_itr != row.end(); ++field_itr) {
      if(!field_itr.IsNull())
          std::cout << (field_itr.IsNull() ? "null" : *field_itr) << ", ";
    }
    std::cout << std::endl;
}

for (auto row : res) {
    for (auto field : row)    // type field: std::string
        std::cout << field << ", ";
    std::cout << std::endl;
}

for (auto& row : res.GetResultSet()) {
    for (auto field : row)    // type field: std::string
        std::cout << field << ", ";
    std::cout << std::endl;
}
```

尽管其它Mode的结果集本身也是 `std::vector` 容器，这里也给出了一个单独的迭代模式的抽象，分为行迭代器 `MysqlRowIterator` 和字段迭代器`MysqlFieldIterator`。且在该模式下 `GetResultSet()` 获取的是 `std::vector<MysqlRow>`  。注意：

- `MysqlRow` 是对结果数据的抽象，并不包含实际数据，实际数据在MysqlResults析构会被释放，对应的 `MysqlRow` 和迭代器均会失效。
- 暂时没有实现对 `auto& row : res` 的引用形式的支持，但可以通过 `GetResultSet()` 获取 `std::vector<MysqlRow>`  来间接实现。
- 在该模式下目前请不要通过 `GetNullFlag` 来判断是否为空，你可以使用以下两种方式：
  1. `bool MysqlRow::IsFieldNull(unsigned int column_index)`
  2. `bool MysqlFieldIterator::bool IsNull()`
- 字段值以 `std::string_view` 返回，可以使用 `MysqlRow::GetFieldData(unsigned int column_index)`  和 `MysqlFieldIterator::operator*()` 获取。



#### class MysqlResults\<Args...\>

```c++
class MysqlResults<Args...>

// example
MysqlResults<int, std::string, MysqlTime> query_res;
proxy->Execute(client_context, query_res,
               "select id, email, created_at from users "
               "where id = 1 or username = \"bob\")";

using ResSetType = std::vector<std::tuple<int, std::string, MysqlTime>>;
if(query_res.IsSuccess()) {
    // ResSetType& res_set = query_res.GetResultSet();
	auto& res_set = query_res.GetResultSet();
    int id = std::get<0>(res_set[0]);
    std::string email = std::get<1>(res_set[1]);
    MysqlTime mtime = std::get<2>(res_set[1]);
}         
else
    std::string error = exec_res.GetErrorMessage()
```

使用类型绑定，如果模板指定为除 `OnlyExec`，`NativeString`，`IterMode` 之外的其它类型，则结果集的每一行是一个tuple，注意模板参数需要和查询结果匹配：

- 如果模板参数个数与实际查询结果字段个数不一致（因此不能用于 "select *"），则可以通过 `GetAffectedRows()` 返回错误
- 如果模板参数的类型和实际字段类型不匹配（例如用int去接受MySQL的字符串），则**目前运行时不会抛出错误(待后续更新)**，用户需要自行保证类型匹配。



## 简单使用示例

### 获取 MysqlServiceProxy 实例

获取一个下游服务的访问代理对象，类型 trpc::redis::RedisServiceProxy。推荐使用 TrpcClient::GetProxy 方式获取。如果你希望单独使用实例，请注意手动初始化，参考 [mysql_service_proxy_test.cc](../../trpc/client/mysql/mysql_service_proxy_test.cc)



#### 初始化配置

```yaml
# ...

client:
  service:
    - name: mysql_server
      protocol: trpc
      timeout: 3000
      network: tcp
      conn_type: long
      is_conn_complex: true
      target: 127.0.0.1:3306
      selector_name: direct
      max_conn_num: 16
      idle_time: 40000

      mysql:
        user_name: "root"          # 数据库用户名
        password: "abc123"         # 用户密码
        dbname: "test"             # 数据库名，一个服务对应一个数据库
        num_shard_group: 2         # 默认为4，设置连接队列shard的组数
        thread_num: 6              # 默认为4，查询任务IO线程池的线程数
        thread_bind_core: true     # 默认true
        enable: true


# ...

```

#### 访问过程

1. 获取proxy, 使用 `::trpc::GetTrpcClient()->GetProxy<HttpServiceProxy>(...)`。

   ```c++
   ::trpc::ServiceProxyOption option;
   
   // ... 此处省略  option 初始化过程
   // 此处的 proxy 获取后在其他地方可继续使用，不需要每次都获取
   auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::mysql::MysqlServiceProxy>("mysql_server", option);
   ```

   通过GetProxy获取实例的使用方法参考 [client_guide](./client_guide.md) ， 三种方式为：

   > 1. 代码中设置 `ServiceProxyOption` 对象。
   > 2. 使用 yaml 配置文件指定 service 配置项。（推荐）
   > 3. 通过回调函数初始化 `ServiceProxyOption` + yaml 配置项。

2. 创建 `ClientContextPtr` 对象 `context`：使用 `MakeClientContext(proxy)`。

   ```cpp
   auto ctx = ::trpc::MakeClientContext(proxy);
   ```

3. 调用 `Query` 。

   ```c++
   MysqlResults<int, std::string> res;
   auto status = proxy->Query(ctx, res, 
                              "select id, username from users where id = ? and username = ?",
                              3, "carol");
   if(!status.OK()) {
     ...
   } else if(!res.IsSuccess()) {
     // std::string error = res.GetErrorMessage();
     ...
   } else {
     ...
   }
   ```

  - 创建一个 `MysqlResults` 来接收结果，作为参数传递给 `Query` 。
  - 传递 SQL 语句，语句中的值变量可以使用 **"?"**  作为占位符，并将占位符对应的变量依次作为参数传递。
  - 最后检查结果。 `MysqlResults` 的使用例子见前文。



## 用户接口

用户可以使用 `trpc::mysql::MysqlServiceProxy` 对 MySQL server进行访问，并可以通过框架 yaml 配置文件对访问进行定制。proxy 支持通过 SQL 语句进行数据库的读写和事务，并同时提供同步和异步两种方式。

| 方法名                     | 描述                                | 参数                                                         | 返回类型                                  | 备注                      |
| -------------------------- | ----------------------------------- | ------------------------------------------------------------ | ----------------------------------------- | ------------------------- |
| `Query`                    | 执行 SQL 查询并检索所有结果行。     | `context`: 客户端上下文;   `res`: 用于返回查询结果的 MysqlResults 对象；  `sql_str`: 带有占位符 "?" 的 SQL 查询字符串  `args`: 输入参数，可以为空 | `Status`                                  |                           |
| `AsyncQuery`               | 异步执行 SQL 查询并检索所有结果行。 | `context`: 客户端上下文；  `sql_str`: 带有占位符 "?" 的 SQL 查询字符串；  `args`: 输入参数，可以为空 | `Future<MysqlResults>`                    |                           |
| `Execute`                  | 同步执行 SQL 查询，用于OnlyExec。   | `context`: 客户端上下文；  `res`: 用于返回查询结果的 MysqlResults 对象；  `sql_str`: 带有占位符 "?" 的 SQL 查询字符串；  `args`: 输入参数，可以为空 | `Status`                                  | 可被`Query` 完全替代      |
| `AsyncExecute`             | 异步执行 SQL 查询，用于OnlyExec。   | `context`: 客户端上下文；  `sql_str`: 带有占位符 "?" 的 SQL 查询字符串；  `args`: 输入参数，可以为空 | `Future<MysqlResults>`                    | 可被`AsyncQuery` 完全替代 |
| `Query`（事务支持）        | 在事务中执行 SQL 查询。             | `context`: 客户端上下文 ； `handle`: 事务句柄；  `res`: 用于返回查询结果的 MysqlResults 对象；  `sql_str`: 带有占位符 "?" 的 SQL 查询字符串；  `args`: 输入参数，可以为空 | `Status`                                  |                           |
| `Execute`（事务支持）      | 在事务中执行 SQL 查询。             | `context`: 客户端上下文；  `handle`: 事务句柄；  `res`: 用于返回查询结果的 MysqlResults 对象；  `sql_str`: 带有占位符 "?" 的 SQL 查询字符串；  `args`: 输入参数，可以为空 | `Status`                                  |                           |
| `AsyncQuery`（事务支持）   | 异步在事务中执行 SQL 查询。         | `context`: 客户端上下文；  `handle`: 事务句柄 ； `sql_str`: 带有占位符 "?" 的 SQL 查询字符串；  `args`: 输入参数，可以为空 | `Future<TransactionHandle, MysqlResults>` |                           |
| `AsyncExecute`（事务支持） | 异步在事务中执行 SQL 查询。         | `context`: 客户端上下文 ； `handle`: 事务句柄；  `sql_str`: 带有占位符 "?" 的 SQL 查询字符串；  `args`: 输入参数，可以为空 | `Future<TransactionHandle, MysqlResults>` |                           |
| `Begin`                    | 开始一个事务。                      | `context`: 客户端上下文；  `handle`: 空事务句柄              | `Status`                                  |                           |
| `Commit`                   | 提交一个事务。                      | `context`: 客户端上下文；  `handle`: 事务句柄                | `Status`                                  |                           |
| `Rollback`                 | 回滚一个事务。                      | `context`: 客户端上下文；  `handle`: 事务句柄                | `Status`                                  |                           |
| `AsyncBegin`               | 异步开始一个事务。                  | `context`: 客户端上下文                                      | `Future<TransactionHandle>`               |                           |
| `AsyncCommit`              | 异步提交一个事务。                  | `context`: 客户端上下文；  `handle`: 事务句柄                | `Future<TransactionHandle>`               |                           |
| `AsyncRollback`            | 异步回滚一个事务。                  | `context`: 客户端上下文；  `handle`: 事务句柄                | `Future<TransactionHandle>`               |                           |

- 当`MysqlResults` 使用 `NativeString` 或 `IterMode` 时，仅仅使用字符串格式化来处理占位符，转为一个完整的SQL语句进行查询。除此之外的情况会使用 MySQL prepared statement，可以防止SQL注入，关于重复使用同一个prepared statement接口目前暂时还未提供。

- 由于mysql api 的调用由线程池完成，因此：

  - 在同步接口中，如果外部逻辑处于fiber协程下，该协程会等待，但不会阻塞线程。
  - 在异步接口中，请注意根据环境选择时使用 `::trpc::future` 里的future相关接口还是 `::trpc::fiber` 里的future相关接口。



## 错误信息

- 同步接口的 `Status` 返回框架错误，`MysqlResult` 通过 `IsSuccess` 和 `GetErrorMessage` 以字符串形式描述MySQL查询错误。因此 `Status` 正常时并不表示 MySQL 查询无错误（例如Timeout）。
- 异步接口如果出错，会返回的 exception future 对象，因为异常future不包含值，因此无法通过 future 里面的 value 即  `MysqlResults` 对象获取MySQL查询错误。因此在异步接口中，不管是框架错误还是MySQL查询错误，均是以异常future的exception返回。



## 更多例子

### 更多类型

对于 `MysqlResults<IterMode>` 和 `MysqlResults<NativeString>` ，不需要关心查询结果的字段类型，因为都是以字符串的形式返回。如果希望将结果绑定到类型，对于日期和Blob，也可以使用 `MysqlTime` 和 `MysqlBlob` (当然你也同样可以使用 `std::string` 作为通用类型来接收)。

#### MysqlTime

```c++
MysqlResults<std::string, MysqlTime, MysqlTime> query_res;
proxy->Query(ctx, query_res, "select event_time, timestamp_col, year_col from users");
auto timestamp_col = std::get<1>(query_res.GetResultSet()[0]);
TRPC_FMT_INFO("year:{}, month:{}, day:{}, hour:{}, second:{}",
             timestamp_col.mt.year,
             timestamp_col.mt.month,
             timestamp_col.mt.day,
             timestamp_col.mt.hour,
             timestamp_col.mt.second);
```

该类型目前只是对 mysql api 中的 `MYSQL_TIME` 包了一层，将其作为成员 `mt`，还未实现易用接口。MySQL中各种日期类型均可以使用 `MysqlTime` 接收，只是结果都被结构化为 year, mouth, day等 （例如如果你想获取原始时间戳，请使用字符串），需要注意其中的无效字段，例如 MySQL 的 **YEAR** 类型，返回的结果中只有 `mt.year` 是有效的，不要使用其它字段。

#### MysqlBlob

```c++
// field "meta" type: BLOB
MysqlResults<std::string, MysqlBlob> query_res;
proxy->Query(ctx, query_res, "select username, meta from users"
                             "where id = ?", 1);
MysqlBlob& blob = std::get<1>(query_res.GetResultSet()[0]);
std::string_view data_view = blob.AsStringView();
```

MysqlBlob底层使用 `std::string` 。

以上两种类型也可以在插入或更新时作为占位符参数传入。



### 插入和更新

```c++
MysqlResults<std::string, MysqlTime> query_res;
MysqlResults<OnlyExec> exec_res;
std::string jack_email = "jack@abc.com";
MysqlTime mtime;
mtime.mt.year = 2024;
mtime.mt.month = 9;
mtime.mt.day = 10;

proxy->Execute(ctx, exec_res,
                         "insert into users (username, email, created_at)"
                         "values (\"jack\", ?, ?)",
                         jack_email, mtime);


ctx = trpc::MakeClientContext(proxy);
proxy->Execute(ctx, exec_res, "delete from users where username = \"jack\"");

```

和select使用方法相同，只是插入和更新语句由于不返回结果集，请使用 `MysqlResult<OnlyExec>`。同样，也可以使用占位符，对应的参数也支持 `MysqlBlob` 和 `MysqlTime` 。

### 事务

使用 `TransactionHandle` 标识一个事务，下面将介绍如何使用 proxy 进行一个事务。

1. **开始一个事务**

   ```c++
   trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
   TransactionHandle handle;
   trpc::Status s = proxy->Begin(ctx, handle);
   
   if(!s.OK()) {
     TRPC_FMT_ERROR("status: {}", s.ToString());
     return;
   }
   ```

   创建一个 `Transaction` 变量，作为 `Begin`参数，来开始一个事务。

2. **在事务中执行查询**

   ```c++
   ...
       
   // Insert a row
   s = proxy->Execute(ctx, handle, exec_res,
                        "insert into users (username, email, created_at)"
                        "values (\"jack\", \"jack@abc.com\", ?)", mtime);
   if(!s.OK() || (exec_res.GetAffectedRows() != 1) || !exec_res.IsSuccess()) {
     TRPC_FMT_ERROR("status: {}, res error: {}", s.ToString(), exec_res.GetErrorMessage());
     return;
   }
   
   // Update a row
   s = proxy->Execute(ctx, handle, exec_res,
                        "update users set email = ? where username = ?",
                        "jack@gmail.com",
                        "jack");
   if(!s.OK() || (exec_res.GetAffectedRows() != 1) || !exec_res.IsSuccess()) {
     TRPC_FMT_ERROR("status: {}, res error: {}", s.ToString(), exec_res.GetErrorMessage());
     return;
   }
   
   ...
   ```

   将 handle 作为第二个参数传入 `Query` 或 `Execute` ，其余和非事务接口一致。

3. **结束事务**

   ```c++
   ...
   s = proxy->Commit(ctx, handle);
   if(!s.OK()) {
     TRPC_FMT_ERROR("status: {}", s.ToString());
     return;
   }
   ...
   ```

   使用 `Commit` 提交一个事务，如果 Status 正常，那么该事务结束，此后handle将不可用；如果 Status 不正常，请检查错误信息后，之后可以再次尝试调用 `Commit` 。

   ```c++
   ...
   s = proxy->RollBack(ctx, handle);
   if(!s.OK()) {
     TRPC_FMT_ERROR("status: {}", s.ToString());
     return;
   }
   ...
   ```

   同样的，也可以 `RollBack` 。

### 异步接口

和同步接口不同，异步接口并非通过传递 `MysqlResult` 来接收查询结果，而是返回 `Future<MysqlResult<Arg...>>` 。其中模板参数在调用 proxy 的函数成员时显示指定

```c++
trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
MysqlResults<int, std::string> res;

using ResultType = MysqlResults<int, std::string>;
auto future = proxy->AsyncQuery<int, std::string>(ctx, "select id, username from users where id = ?", 3)
                        .Then([](trpc::Future<ResultType>&& f){
                          if(f.IsReady()) {
                            auto res = f.GetValue0();  // MysqlResults<int, std::string>
                            printResult(res.GetResultSet());
                            return trpc::MakeReadyFuture();
                          }
                          return trpc::MakeExceptionFuture<>(f.GetException());
                        });
std::cout << "do something\n";
trpc::future::BlockingGet(std::move(future));
```

这里 `proxy->AsyncQuery<int, std::string>(ctx, "select id, username from users where id = ?", 3)` 应当返回 `Future<MysqlResults<int, std::string>>` ，然后通过 `Then`  回调处理查询结果， 将 `AsyncQuery` 返回的 future 中的值获取出来，然后打印，最后返回一个就绪的空future。

**对于异步事务接口**，参考 [future_client.cc](../../examples/features/mysql/client/future/future_client.cc)

```c++
MysqlResults<OnlyExec> exec_res;
MysqlResults<IterMode> query_res;
TransactionHandle handle;

trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);

auto fu = proxy->AsyncBegin(ctx)
          .Then([&handle](Future<TransactionHandle>&& f) mutable {
            if(f.IsFailed())
              return trpc::MakeExceptionFuture<>(f.GetException());
            handle = f.GetValue0();  // move assignment
            return trpc::MakeReadyFuture<>();
          });

trpc::future::BlockingGet(std::move(fu));

auto fu2 = proxy
      ->AsyncQuery<IterMode>(ctx, std::move(handle), "select username from users where username = ?", "alice")
      .Then([](Future<TransactionHandle, MysqlResults<IterMode>>&& f) mutable {
        if(f.IsFailed())
          return trpc::MakeExceptionFuture<TransactionHandle>(f.GetException());
        auto t = f.GetValue();
        auto res = std::move(std::get<1>(t));

        std::cout << "\n>>> select username from users where username = alice\n";
        PrintResultTable(res);
        return trpc::MakeReadyFuture<TransactionHandle>(std::move(std::get<0>(t)));
      });

auto fu3 = trpc::future::BlockingGet(std::move(fu2));

...
```

通过 `AsyncBegin` 开始一个事务，它会返回 `Future<TransactionHandle>` ，这里使用一个 `handle` 在回调里取出返回的 Transactionhandle，然后赋值给它（事实上也可以不用回调，直接在外面去Get里面的handle）。之后为了在事务中进行查询，我们需要传递 handle 参数给 `AsyncQuery` 和`AsyncExecute` 。后续在事务上的所有操作只需要传递 handle 即可。

只要有 handle，后续这个事务也可以使用同步事务接口。

你也可以直接在Then Chain里面完成整个事务，详情见  [future_client.cc](../../examples/features/mysql/client/future/future_client.cc)

