This directory contains the implementation of tRPC-Cpp Future/Promise.
# Design Principle
- Future/Promise both support move semantics, but forbid copy semantics, to make sure Promise and Future is 1:1.
- Future/Promise support variable arguments, so the result type of Future is std::tuple, users need to invoke std::get\<N\> to obtain the Nth argument result. On the other hand, with the frequent use of single argument version of Future/Promise, we do template specialization to utilize better performance.
- Support callback argument type of Future or value to Future Then function.
- Instead of throwing exception, Exception object is returned when Future/Promise encounters error, users can invoke GetException to get the error message.
- Users can implement their own Executor to specify how the Future callback is invoked.
- Future schedule is based on the Continuation theory, which is introduced in [Continuation](https://en.wikipedia.org/wiki/Continuation).

# Usage
Future and Promise class are all the users need to pay attention to, as showed by the demo below.
```
#include <iostream>

#include "trpc/future/future.h"

int main() {
  // Create Promise instance.
  trpc::Promise<std::string> pr;
  // Get corresponding Future instance.
  auto fut = prom.GetFuture();
  // Register callback to the future.
  fut.Then([](trpc::Future<std::string>&& fut){
    // Check the future is ready or exceptional.
    if (fut.IsReady()) {
      // Use GetValue0 in case of single argument version.
      auto value = fut.GetValue0();
      std::cout << "message = " << message << std::endl;
    } else {
      // Get exception here.
      auto exception = fut.GetException();
      std::cout << "exception = " << exception.what() << std::endl;
    }
    // Chained future can be returned.
    return trpc::MakeReadyFuture<>();
  });

  // Set the ready result of future through promise.
  pr.SetValue(std::string("hello,world"));
  return 0;
}
```
Above demo finally print `message = hello,world`.

# Attentions
- For the sake of poor performance, users should use Future Wait carefully, use async programming through Then callback instead.
- IsReady/IsFailed must be invoked before GetVaule and GetException. By the way, GetVaule or GetException must be inoked at most one time in case of one future.
- The lifetime of Executor must be longer than future callback, if provided by users.
