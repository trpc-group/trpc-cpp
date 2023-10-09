[TOC]

# Instructions for using RedisServiceProxy

## 1 RedisServiceProxy
 `RedisServiceProxy` is an extension of `ServiceProxy` for the `redis` protocol.

## 2 User interface 
### 2.1 Synchronous calling interface.
#### 2.1.1 Status Command(const ClientContextPtr& context, Reply* reply, const std::string& cmd)
Method: `Command`. Use this interface send Redis command(such as 'set'、'get' ) with param 'string cmd' to Redis Server.
#### 2.1.2 Status Command(const ClientContextPtr& context, Reply* reply, std::string&& cmd)
Method: `Command`. Same as above interface which param 'cmd' is right value.
#### 2.1.3 Status Command(const ClientContextPtr& context, Reply* reply, const char* format, ...)
Method: `Command`. Same as above interface which command can be formatted style,for example:Command(context, reply, "get %s", usr_name.c_str())
#### 2.1.4 Status CommandArgv(const ClientContextPtr& context, const Request& req, Reply* reply)
Method: `Command`. Use this interface send Redis command(such as 'set'、'get' ) with param 'Request req' to Redis Server.
#### 2.1.5 Status CommandArgv(const ClientContextPtr& context, Request&& req, Reply* reply)
Method: `Command`, Same as above interface which param 'Request req' is right value.

### 2.2 Asynchronous calling interface
#### 2.2.1 Future<Reply> AsyncCommand(const ClientContextPtr& context, const std::string& cmd)
Method: `AsyncCommand`. Use this interface send Redis command(such as 'set'、'get' ) with param 'string cmd' to Redis Server.
#### 2.2.2 Future<Reply> AsyncCommand(const ClientContextPtr& context, std::string&& cmd)
Method: `AsyncCommand`. Same as above interface which param 'string cmd' is right value.
#### 2.2.3 Future<Reply> AsyncCommand(const ClientContextPtr& context, const char* format, ...)
Method: `AsyncCommand`. Same as above interface which command can be formatted style,for example:Command(context, reply, "get %s", usr_name.c_str()).
#### 2.2.4 Future<Reply> AsyncCommandArgv(const ClientContextPtr& context, const Request& req)
Method: `AsyncCommand`. Use this interface send Redis command(such as 'set'、'get' ) with param 'Request req' to Redis Server.
#### 2.2.5 Future<Reply> AsyncCommandArgv(const ClientContextPtr& context, Request&& req)
Method: `AsyncCommand`, Same as above interface which param 'Request req' is right value.

### 2.3 Oneway calling interface
Oneway means do not handle the reply from the server,it just send request. 
#### 2.3.1 Status Command(const ClientContextPtr& context, const std::string& cmd)
Method: `Command`. Use this interface send Redis command(such as 'set'、'get' ) with param 'string cmd' to Redis Server.
#### 2.3.2 Status Command(const ClientContextPtr& context, const std::string& cmd)
Method: `Command`. Same as above interface which param 'cmd' is right value.
 
### 3 Example
A typical `Command` request:
```cpp

std::shared_ptr<ServiceProxyOption> authed_redis_proxy_option = std::make_shared<ServiceProxyOption>();
authed_redis_proxy_option->name = "authed_redis_client";
authed_redis_proxy_option->codec_name = "redis";
authed_redis_proxy_option->timeout = 1000;
authed_redis_proxy_option->selector_name = "direct";
authed_redis_proxy_option->target = "127.0.0.1:6379";
authed_redis_proxy_option->redis_conf.enable = true;
authed_redis_proxy_option->redis_conf.password = "your_password";

std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
    trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("authed_redis_client",
                                                                    authed_redis_proxy_option.get());

trpc::redis::Reply reply;
::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(authed_redis_proxy_);
::trpc::Status status = authed_redis_proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.get("redis_key"));
if (!status.OK()) {
  // rpc fail,maybe timeout or connect failed.
  std::cout << "Invoke redis get command fail: " << status.ErrorMessage() << std::endl;
} else if (reply.IsError()) {
  // get error from redis-server
  std::cout << "Invoke redis get command fail, error msg: " << reply.GetString() << std::endl;
} else if (reply.IsNil()) {
  // get Nil from redis-server
  std::cout << "Invoke redis get command return Nil " << std::endl;
} else { 
  // get redis value success
  std::cout << "Invoke redis get command success. reply: " << reply << std::endl;
}
```