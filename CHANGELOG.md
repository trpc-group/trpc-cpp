# Change Log

## [1.1.0](https://github.com/trpc-group/trpc-cpp/releases/tag/v1.1.0)(2023-12-28)

### Features And Improvements
- **fiber:**  add adaptive primitives for both fiber and pthread context (#94) @hzlushiliang
- **fiber:**  optimize fiber primitives to reduce fiber context switch (#87) @helloopenworld
- **redis:**  support redis_lpop_count command (#69) @yujun411522
- **util:**  use RoundUpPowerOf2() in LockFreeQueue Init() (#93) @ypingcn

### Bug Fixes
- **transport:** fix fd double close when accept over max_conn_num limit (#73) @yujun411522
- **https:** fix protocol check error of bad magic number due to ssl write dirty data to reused socket fd (#96) @liucf3995
- **future:** fix prossible timeout in low qps situation in future transport when using connection reuse mode (#85) @chhy2009
- **stream:** fix crash when peer send reset stream frame but has no error code (#82) @weimch
- **plugin:** fix the issue of incomplete initialization of plugins with the same name but different types (#92) @chhy2009
- **config:** fix coredump of trpc::config::GetInt64() with overflowed int32 (#89) @liucf3995
- **logging:** fix TRPC_STREAM do not have context (#84) @raychen911
- **logging:** fix ReadTsc to avoid infinite loop in kNanosecondsPerUnit initialization (#97) @hzlushiliang
- **util:** fix NoncontiguousBuffer.Find() fails to find substring in some cases (#90) @NewbieOrange

### Other Changes
- **doc:** Add/improve documents @raychen911 @weimch


## [1.0.0](https://github.com/trpc-group/trpc-cpp/releases/tag/v1.0.0)(2023-10-27)

tRPC-Cpp first version