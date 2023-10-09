# Flow control

## Configured through the framework

~~~yaml
server:
  service: #Business service, can have multiple.
    - name: trpc.test.helloworld.Greeter #Service name, needs to be filled in according to the format, the first field is default to trpc, the second and third fields are the app and server configurations above, and the fourth field is the user-defined service_name.
      service_limiter: default(100000) #Service-level flow control limiter, standard format: name (maximum limit per second), empty for no limit.
      func_limiter: #Interface-level flow control.
        - name: SayHello #Method name
          limiter: default(100000) #Interface-level flow control limiter, standard format: name (maximum limit per second), empty for no limit.
        - name: Route #Method name
          limiter: default(100000) #Interface-level flow control limiter, standard format: name (maximum limit per second), empty for no limit.
~~~

Currently supported flow control limiter names are: `default`, `seconds`, `smooth`

## Interface traffic restriction.

Business users need to register flow control limiters for the interfaces that require traffic control.

~~~cpp
trpc::FlowControllerPtr say_hello_controller(new trpc::SmoothLimter(10000, 100));
trpc::FlowControllerFactory::GetInstance()->Register(
    "/trpc.test.helloworld.Greeter/SayHello", 
    say_hello_controller);
~~~

Then limit the service to a maximum of 10,000 requests per second for this interface on a single machine.

## Service-level flow control limiter

Business users register service traffic flow control limiters, and the name of the service traffic flow control limiter is the service name, such as: `trpc.test.helloworld.Greeter`

~~~cpp
trpc::FlowControllerPtr service_controller(new trpc::SmoothLimter(200000, 100));
trpc::FlowControllerFactory::GetInstance()->Register(
    "trpc.test.helloworld.Greeter", 
    service_controller);
~~~

Then limit the service to a maximum of 200,000 requests per second for this interface on a single machine.

# Trigger conditions.

First, check the service traffic restriction. If the service traffic restriction is exceeded, return a flow control error. Then check the interface traffic restriction. If the limit is exceeded, return a flow control error.

# Implementation principle.

1. Traffic control is to limit the total number of requests that can be accepted per second.

2. Divide 1 second into N time slices, and only store the cumulative number of requests within each time slice. The total number of current requests per second is the cumulative count of N time slices in the last 1 second.
3. By using the concept of "sliding window", the system triggers the sliding of the current time window every 1/N second:
   The oldest time slice is discarded, and a new time slice is added (with the count reset to 0).
   First:
   [1,2,3,..,N]

   Second:
      [2,3,...,N+1]

   Third:
        [3,4,...,N+2]

   The sliding window keeps sliding within a circular queue, which enables the storage of request counts in all time slices within the last 1 second.

4. By triggering the system at a fixed interval (1/N second), the smooth passage of time and the accumulation of requests within the last 1 second can be achieved.
