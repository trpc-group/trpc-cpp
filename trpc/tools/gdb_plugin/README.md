# Debugging fibers using gdb

We provide a gdb plugin to view the fiber stack of **running processes** as well as **those in coredumps**.

Prerequisites: GDB version 7.6 or above is required, and the program compilation needs to enable the "-g" option, if compiling with bazel, the corresponding option to add is "--copt=-g --strip=never". Then, add the configuration item enable_gdb_debug: true in the framework settings to enable it:

```
global:
  threadmodel:
    fiber:
      - instance_name: fiber_instance
        enable_gdb_debug: true
```


Usage：
1. Execute `gdb -p pid` or `gdb prog core` to enter the gdb command-line interface (CLI).
2. Run `source /path/to/trpc/fiber/gdb_fiber_plugin.py` in the gdb CLI to load the plugin.
3. Afterwards, you can enumerate the running fibers in GDB using the following command:
    ```
    list-fibers: Enumerate the fibers in the process.
    list-fibers-compact: Similar to list-fibers, but it merges fibers with the same call stack before outputting, making it easier to observe.
    ```

The output is as follows:
```
(gdb) source trpc/tools/gdb_plugin/gdb_fiber_plugin.py
(gdb) info fibers
  Id    Frame
Found 3392 stacks in total, checking for fiber aliveness. This may take a while.

   2    0x00007f5a416df0e3 epoll_wait + 51 [(Unknown)]
   1    0x00000000009249d4 trpc::fiber::detail::FiberEntity::ResumeOn(trpc::Function<void ()>&&) + 148 [./trpc/runtime/threadmodel/fiber/detail/fiber_entity.h:389]
(gdb) list-fibers
Found 3392 stacks in total, checking for fiber aliveness. This may take a while.

Fiber #2:
RIP 0x00007f5a416df0e3 RBP 0x00007f598e6feee0 RSP 0x00007f598e6fee90
#0 0x00007f5a416df0e3 epoll_wait + 51 [(Unknown)]
#1 0x0000000000910915 trpc::FiberReactor::Run() + 53 [trpc/runtime/iomodel/reactor/fiber/fiber_reactor.cc:63]
#2 0x0000000000910a92 trpc::Function<void ()>::ErasedCopySmall<trpc::fiber::StartAllReactor()::{lambda()#1}&>(void*, trpc::fiber::StartAllReactor()::{lambda()#1}&)::{lambda(trpc::Function<void ()>::TypeOps const*)#1}::_FUN(trpc::Function<void ()>::TypeOps const*) + 114 [trpc/runtime/iomodel/reactor/fiber/fiber_reactor.cc:184]
#3 0x0000000000924aab trpc::fiber::detail::FiberProc(void*) + 123 [trpc/runtime/threadmodel/fiber/detail/fiber_entity.cc:102]

Fiber #1:
RIP 0x00000000009249d4 RBP 0x00007f598e5fe8d0 RSP 0x00007f598e5fe880
#0 0x00000000009249d4 trpc::fiber::detail::FiberEntity::ResumeOn(trpc::Function<void ()>&&) + 148 [./trpc/runtime/threadmodel/fiber/detail/fiber_entity.h:389]
#1 0x0000000000931faa trpc::fiber::detail::v1::SchedulingImpl::Suspend(trpc::fiber::detail::FiberEntity*, std::unique_lock<trpc::Spinlock>&&) + 154 [/opt/rh/devtoolset-8/root/usr/lib/gcc/x86_64-redhat-linux/8/../../../../include/c++/8/new:169]
#2 0x000000000091ff00 trpc::fiber::detail::WaitableTimer::wait() + 144 [trpc/runtime/threadmodel/fiber/detail/waitable.cc:168]
#3 0x0000000000913c92 trpc::FiberSleepUntil(std::chrono::time_point<std::chrono::_V2::steady_clock, std::chrono::duration<long, std::ratio<1l, 1000000000l> > > const&) + 34 [trpc/coroutine/fiber.cc:155]
#4 0x0000000000913cd1 trpc::FiberSleepFor(std::chrono::duration<long, std::ratio<1l, 1000000000l> > const&) + 33 [trpc/coroutine/fiber.cc:159]
#5 0x000000000074a0f3 trpc::TrpcServer::WaitForShutdown() + 67 [/opt/rh/devtoolset-8/root/usr/lib/gcc/x86_64-redhat-linux/8/../../../../include/c++/8/bits/std_function.h:260]
#6 0x00000000005fcd4f trpc::TrpcApp::Execute() + 2335 [trpc/common/trpc_app.cc:130]
#7 0x00000000008fc9e9 trpc::Function<void ()>::ErasedCopySmall<trpc::runtime::Run(std::function<void ()>&&)::{lambda()#2}>(void*, trpc::runtime::Run(std::function<void ()>&&)::{lambda()#2}&&)::{lambda(trpc::Function<void ()>::TypeOps const*)#1}::_FUN(trpc::Function<void ()>::TypeOps const*) + 201 [trpc/runtime/runtime.cc:114]
#8 0x0000000000924aab trpc::fiber::detail::FiberProc(void*) + 123 [trpc/runtime/threadmodel/fiber/detail/fiber_entity.cc:102]

Found 2 fiber(s) in total.
```

Note: for the production environment, you can use the following command to minimize the impact of debugging on the running service (although this command can still cause a second-level service suspension): `gdb --pid <PID> --eval-command='source gdb_fiber_plugin.py' --eval-command='set pagination off' --eval-command='list-fibers' --batch`.
