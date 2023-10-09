# Overload module

Includes five overload protection strategies.

## 1、concurrency_limiter

Overload protection strategy based on concurrent requests, *Non-adaptive algorithm*.

- **Currently, only server-side rate limiting is supported.**
- **Users need to ensure that ServerContext can be destructed when using it. If the business scenario requires that ServerContextPtr not be released, this feature cannot be used.**

## 2、fiber_limiter

Overload protection strategy based on the number of fibers, *Non-adaptive algorithm*.

- Support both **client-side and server-side**.
- The number of fibers obtained **may not be exact**.

## 3、flow_control

Overload protection strategy based on traffic control, *Non-adaptive algorithm*.

- **only server-side rate limiting is supported.**
- Includes two algorithms: **sliding window and fixed window.**

## 4、high_percentile

Overload protection strategy based on EMA algorithm with high percentile, *Adaptive algorithm*.

- **Currently, only server-side rate limiting is supported.**

## 5、throttler

Overload protection strategy based on EMA algorithm with downstream success rate, *Adaptive algorithm*.

- **Currently, only client-side rate limiting is supported.**
