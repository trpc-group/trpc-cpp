selector_load_balance.h
定义了一个名为 TestPollingLoadBalance 的类，它继承自 Selector 类，用于实现负载均衡的策略。
TestPollingLoadBalance 类
这个类实现了负载均衡策略，继承自 Selector，具体包括：
- 名称和版本：
std::string Name() const override { return "test_polling_load_balance"; }
std::string Version() const override { return "0.0.1"; }
  提供了插件的名称和版本。
- 初始化：
int Init() noexcept override;
  初始化插件，返回成功（0）或失败（-1）。
- 选择服务节点：
int Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) override;
Future<TrpcEndpointInfo> AsyncSelect(const SelectorInfo* info) override;
  选择服务节点的同步和异步方法。
- 批量选择节点：
int SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) override;
Future<std::vector<TrpcEndpointInfo>> AsyncSelectBatch(const SelectorInfo* info) override;
  批量选择节点的同步和异步方法。
- 报告调用结果：
int ReportInvokeResult(const InvokeResult* result) override;
  报告调用结果，返回成功（0）或失败（-1）。
- 设置路由信息：
int SetEndpoints(const RouterInfo* info) override;
  设置服务的路由信息，返回成功（0）或失败（-1）。
私有成员
- EndpointsInfo
包含节点信息和节点 ID 生成器。
- 数据成员：
std::unordered_map<std::string, EndpointsInfo> targets_map_;
LoadBalancePtr default_load_balance_ = nullptr;
std::shared_mutex mutex_;
  用于存储目标信息、默认负载均衡器和互斥锁。

单元测试selector_load_balance_test.cc
实现了对 SmoothWeightedPollingLoadBalance 插件的单元测试
1. 测试类结构 
- 成员变量：
  - load_balance_：指向 SmoothWeightedPollingLoadBalance 的智能指针
  - test_polling_load_balance_：指向 TestPollingLoadBalance 的智能指针，用于辅助测试负载均衡逻辑的插件。
- SetUp()
  - 初始化并启动 PeripheryTaskScheduler
  - 创建 SmoothWeightedPollingLoadBalance 实例并将其传递给 TestPollingLoadBalance 进行初始化和启动
- TearDown()
  - 停止并销毁 test_polling_load_balance_。
  - 停止并等待 PeripheryTaskScheduler 的任务完成。
2.测试用例
- TestSingleSelectionAccuracy：
  - 验证单个服务端点的选择准确性。通过设置一个端点（192.168.1.1:1001，权重10），测试了同步选择、异步选择和批量选择的正确性。
- SelectWithWeights：
  - 验证带权重的选择准确性。通过设置两个不同权重的端点，测试了 1000 次选择的分布情况。
  - 统计端点选择次数，并检查其比例是否符合预期（即权重比例 2:1）。
- AsyncSelectWithWeights：
  - 创建了多个异步选择任务，并在完成后验证每个选择结果的正确性。
  - 同样统计选择次数，并验证权重比例是否符合预期。
- SelectBatchWithWeights：
  - 验证批量选择的准确性。通过设置多个端点并进行批量选择，测试了在 SelectorPolicy::MULTIPLE 策略下的表现。
  - 确保批量选择返回的端点数量大于等于 1，并且其比例符合端点的权重分布。
-  AsyncSelectBatchWithWeights 
  - 验证 SmoothWeightedPollingLoadBalance 插件中的异步批量选择功能是否能够按照配置的权重正确选择端点
- DynamicEndpointAddRemove 
  - 验证在 SmoothWeightedPollingLoadBalance 插件中动态添加和删除服务端点时负载均衡器的行为


result:
Running main() from gmock_main.cc
[==========] Running 6 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 6 tests from SmoothWeightedPollingLoadBalanceTest
[ RUN      ] SmoothWeightedPollingLoadBalanceTest.TestSingleSelectionAccuracy
Router info of name test_service1 not found
Failed to perform load balance for test_service1
Router info for test_service1 not found.
[       OK ] SmoothWeightedPollingLoadBalanceTest.TestSingleSelectionAccuracy (1 ms)
[ RUN      ] SmoothWeightedPollingLoadBalanceTest.SelectWithWeights
[       OK ] SmoothWeightedPollingLoadBalanceTest.SelectWithWeights (0 ms)
[ RUN      ] SmoothWeightedPollingLoadBalanceTest.AsyncSelectWithWeights
[       OK ] SmoothWeightedPollingLoadBalanceTest.AsyncSelectWithWeights (1 ms)
[ RUN      ] SmoothWeightedPollingLoadBalanceTest.SelectBatchWithWeights
[       OK ] SmoothWeightedPollingLoadBalanceTest.SelectBatchWithWeights (0 ms)
[ RUN      ] SmoothWeightedPollingLoadBalanceTest.AsyncSelectBatchWithWeights
[       OK ] SmoothWeightedPollingLoadBalanceTest.AsyncSelectBatchWithWeights (0 ms)
[ RUN      ] SmoothWeightedPollingLoadBalanceTest.DynamicEndpointAddRemove
[       OK ] SmoothWeightedPollingLoadBalanceTest.DynamicEndpointAddRemove (1 ms)
[----------] 6 tests from SmoothWeightedPollingLoadBalanceTest (7 ms total)

[----------] Global test environment tear-down
[==========] 6 tests from 1 test suite ran. (7 ms total)
[  PASSED  ] 6 tests.


