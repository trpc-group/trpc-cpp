[中文](../zh/backup_request.md)

# Overview

Sometimes, to ensure availability or reduce tail latency, it is necessary to simultaneously access two services and take the response from whichever arrives first. There are generally two approaches to implementing this functionality:

1. Concurrently send two requests and take the result from the one that returns first.
2. Set a reasonable resend time. If a request times out or fails
within the resend time, send the second request and take the result from the one that returns first.

One issue with approach 1 is that it doubles the backend traffic. Approach 2, on the other hand, typically results in only one request under normal circumstances, keeping the backend traffic mostly unchanged. Considering this, we chooses to implement approach 2.

In addition, considering that retrying requests when the backend service is already overloaded or experiencing abnormalities can worsen the situation, the framework also provides a flow control strategy that automatically terminates retries based on the success or failure of the request. For more details, refer to [Automatically cancel retries based on the call result](#automatically-cancel-retries-based-on-the-call-result).

# Invocation process

![img](../images/backup_request.png)

1. When invoked, two different backends are selected through the naming module, and then the request is sent to the transport layer.
2. The transport layer sets the timeout duration as the resend time and sends the request to backend 1, waiting for the response.
3. If the request is successful within the resend time, it returns and ends the invocation.
4. If the request times out or fails within the resend time, a resend is triggered. The request is sent to backend 2, and then the result is awaited. (If the first request times out, its timeout duration is modified to the remaining time (request timeout - resend time), and the faster response between backend 1 and backend 2 is taken.)

The entire calling process is transparent to the user. Users only need to enable backup request and then initiate the normal call.

# Usage

## Enable backup request

The API interfaces related to backup request are located in [ClientContext](../../trpc/client/client_context.h), as follows:

```cpp
void SetBackupRequestDelay(uint32_t delay);

void SetBackupRequestAddrs(const std::vector<NodeAddr>& addrs);
```

`SetBackupRequestDelay` is used to specify the resend time. Calling this interface is considered as enabling backup request. And `SetBackupRequestAddrs` is used for users to manually set the backend nodes to be called.
In the case of using the selector plugin, the backend nodes are automatically selected by the selector plugin, so you only need to call `SetBackupRequestDelay`. For scenarios where users need to manually set the backend nodes to be called, it is required to call `SetBackupRequestDelay` first and then call `SetBackupRequestAddrs` to set the backends.

A demo is as follows:

```cpp
// set backup-request resend time as 10ms
client_context->SetBackupRequestDelay(10);
// send request
auto status = service_proxy->SayHello(client_context, request, &reply);
```

## Automatically cancel retries based on the call result

To avoid additional traffic impact caused by backup requests when the backend service is overloaded or experiencing abnormalities, we have implemented a retry rate limiting filter called 'retry_hedging_limit' to handle such scenarios.

The specific code implementation can be seen in [RetryLimitClientFilter](../../trpc/filter/retry/retry_limit_client_filter.h). The algorithm implementation uses a token bucket rate limiting mechanism, where the token count in the bucket is initialized as max_tokens. On a successful request, the token count increases by 1 until it reaches max_tokens. On a failed request, the token count decreases by N (token_ratio) until it goes below 0. The retry/hedging strategy only takes effect when the token count in the bucket is greater than half of its capacity. When the token count is less than or equal to half of the capacity, retries are canceled.

This feature is not enabled by default. Users can enable it through the following configuration file method:

```yaml
client:
  service:
    - name: trpc.test.helloworld.Greeter
      ...
      filter:
        - retry_hedging_limiter  # use `retry_hedging_limiter` filter
      filter_config:  # configuration of service-level filter
        retry_hedging_limiter:  # configuration of `retry_hedging_limiter` filter
          max_tokens: 20  # max token number in bucket, default value is 100
          token_ratio: 2  # The ratio between the number of tokens decreased for each failed request and the number of tokens increased for each successful request (increased by 1 on success), which represents the penalty factor for failures. It is of type 'int' and has a default value of 10.
```

Or by specifying it in the code:

```cpp
#include "trpc/common/config/retry_conf.h"
...
trpc::ServiceProxyOption option;
option.name = "trpc.test.helloworld.Greeter";
// add retry limit filter
option.service_filters.push_back(trpc::kRetryHedgingLimitFilter);

trpc::RetryHedgingLimitConfig config;
config.max_tokens = 20;
config.token_ratio = 2;
option.service_filter_configs[trpc::kRetryHedgingLimitFilter] = config;
proxy = trpc::GetTrpcClient()->GetProxy<...>(option.name, &option);
```

When enabled, during a client call, it will first check if a retry is possible. If the retry condition is not met (current token count < max_tokens/2), the retry will be canceled.

If the strategy of the 'retry_hedging_limit' retry rate limiting filter does not meet the requirements, you can also implement your own rate limiting filter and register it to framework for use (either as a service-level filter or a global filter, depending on the situation). The registration and usage of filters can be referred to in the [Customize filters](filter.md).

## View the triggering results of backup requests

The framework provides two tvar variables related to backup requests internally (where service_name is the name of the called service):

- trpc/client/service_name/backup_request: indicate how many times the backup request has been triggered
- trpc/client/service_name/backup_request_success: represent the number of times the backup request has resulted in a successful invocation

These two variables can be viewed using management commands:

```shell
curl http://admin_ip:admin_port/cmds/var/xxx # xxx represent tvar variables
```

It also supports reporting attribute monitoring to the corresponding platform.

# Notes

1. Interface idempotence: Users need to ensure that the RPC calls they use are idempotent across different nodes.
2. Before enabling backup requests, it is important to ensure that the backend service has sufficient processing capacity to avoid triggering a service avalanche when backup requests are made.
3. It is necessary to select an appropriate resend time to reduce call latency while avoiding excessive traffic impact on the backend service. Usually, metrics such as P90/P95/P99 latency can be used as a basis for selection.
4. Backup requests are not suitable for UDP, one-way calls, or streaming calls.
5. If there is only one available node in the backend service or the set resend time is greater than the request timeout, the backup request will degrade into a normal one-to-one call.
