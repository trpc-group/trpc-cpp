# Change Log

## [1.2.0](https://github.com/trpc-group/trpc-cpp/releases/tag/v1.2.0)(2024-07-02)

### Features And Improvements
- **protobuf:** support pb-lite seriazation (#153) @weimch
- **overload:** add abstract and factory classes for overload controller module (#134) @weimch
- **client:** optimize client class or datastruct, read wait-free (#127) @helloopenworld
- **document:** update the stub code generation of Protobuf IDL file in trpc_protocol_service.md (#126) @chhy2009
- **server:** more precise control over the readability and writability events of connections to reduce timeouts during graceful shutdown (#125) @yujun411522
- **codec:** use trpc protocol at remote side (#124) @weimch
- **testing:** add unit-testing building rules for trpc/util/algorithm (#120) @liucf3995
- **threadmodel:** modify the key of scheduling_map to std::string to avoid accessing SeparateScheduling as nullptr during startup (#118) @yujun411522
- **style:** add code inspection to the CI workflow (#116) @bochencwx
- **fiber:** support FiberCount attribute report (#117) @hzlushiliang
- **redis:** redis reply change from union to variant (#115) @yujun411522
- **runtime:** add runtime state management to avoid duplicate calls (#112) @yujun411522
- **style:** correct the spelling error (#110) @iutx

### Bug Fixes
- **util:** fix TransformConfig return true convert not valid json-string to json-object (#153) @weimch
- **transport** fix connection block problem when WritingBufferList at high payload (#153) @weimch
- **http:** fix the issue where metrics reports as correct when the HTTP client's asynchronous call returns an erroneous HTTP response code (#152) @bochencwx
- **http:** fix the issue of rpcz not correctly obtaining the size of the HTTP client response packet (#152) @bochencwx
- **http:** fix the case sensitivity issue in key comparison for HttpHeader.SetIfNotPresent interface (#152) @bochencwx
- **http:** fix connection can't send data while http async stream writing empty data (#146) @weimch
- **filter:** fix the problem that the CLIENT_POST_SCHED_RECV_MSG filter point is not executed in case of decoding failure (#151) @chhy2009
- **admin:** fix the issue of deregistering AdminService when the service exits in self-register case. (#145) @chhy2009
- **compile:** fix error: undefined reference to '_dl_sym' when link libtrpc_Sadmin_Slibmutex.so (#133) @zhsnew
- **asyncio:** fix the problem that AsyncIO fails to correctly handle the return value when using SQ_POLL (#132) @yanyupeng2018
- **transport:** When accepting a connection, if the created file descriptor (FD) is 0, 1, or 2, avoid core dump (#129) @yujun411522
- **serialization:** fix pb derialization failed at server (#114) @weimch
- **backup request:** fix the problem of uneven traffic using backup request in IP direct connection scenario (#106) @chhy2009
- **transport:** fix the issue of increasing number of close-wait connection when using fiber connection pool (#108) @liucf3995
- **http:** fix the issue that the unary HTTP server occasionally fails to trigger the business logic at fiber thread model (#105) @bochencwx
- **stream:** fix trpc stream crash at client when connect failed with high frequency (#101) @weimch
- **http:** fix the issue that the user fails to read data in the scenario where an HTTP chunked client encounters a disconnection immediately after completing data reception (#100) @bochencwx

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