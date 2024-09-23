#pragma once

#include "trpc/client/mysql/executor/mysql_executor.h"
#include "trpc/client/mysql/mysql_executor_pool.h"
#include "trpc/client/mysql/mysql_executor_pool_manager.h"
#include "trpc/client/service_proxy.h"
#include "trpc/coroutine/fiber_event.h"
#include "trpc/util/thread/latch.h"
#include "trpc/util/thread/thread_pool.h"

namespace trpc::mysql {

class MysqlServiceProxy : public ServiceProxy {
 public:
  MysqlServiceProxy();

  ~MysqlServiceProxy() override {
    thread_pool_->Stop();
    thread_pool_->Join();
  }

  template <typename... OutputArgs, typename... InputArgs>
  Status Query(const ClientContextPtr& context, MysqlResults<OutputArgs...>& res, const std::string& sql_str,
               const InputArgs&... args);

  template <typename... OutputArgs, typename... InputArgs>
  Future<MysqlResults<OutputArgs...>> AsyncQuery(const ClientContextPtr& context, const std::string& sql_str,
                                                 const InputArgs&... args);

  void ServiceOptionToMysqlConfig();
  bool SplitAddressPort(const std::string& address, std::string& ip, uint32_t& port);

 private:
  std::unique_ptr<ThreadPool> thread_pool_{nullptr};
  // std::unique_ptr<MysqlExecutorPoolManager> pool_manager_;
  std::vector<MysqlClientConf*> vec_;
};

template <typename... OutputArgs, typename... InputArgs>
Status MysqlServiceProxy::Query(const ClientContextPtr& context, MysqlResults<OutputArgs...>& res,
                                const std::string& sql_str, const InputArgs&... args) {
  // Adaptive event primitive for both fiber and pthread context
  FiberEvent e;
  // auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  thread_pool_->AddTask([this, &e, &res, &sql_str, &args...]() {
    auto conn = MysqlExecutorPoolManager::getPool(vec_[0])->getConnection();

    // sleep(2);
    e.Set();
  });

  e.Wait();
  // RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  return Status();
}

template <typename... OutputArgs, typename... InputArgs>
Future<MysqlResults<OutputArgs...>> MysqlServiceProxy::AsyncQuery(const ClientContextPtr& context,
                                                                  const std::string& sql_str,
                                                                  const InputArgs&... args) {
  Promise<MysqlResults<OutputArgs...>> pr;
  auto fu = pr.GetFuture();

  // auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  // if (filter_status == FilterStatus::REJECT) {
  //   TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
  //   context->SetRequestData(nullptr);
  //   const Status& result = context->GetStatus();
  //   auto exception_fut =
  //   MakeExceptionFuture<MysqlResults<OutputArgs...>>(CommonException(result.ErrorMessage().c_str()));
  //   filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  //   return exception_fut;
  // }

  thread_pool_->AddTask([p = std::move(pr), sql_str, args...]() mutable {
    MysqlResults<OutputArgs...> res;
    // sleep(1);

    // Get address from `context->endpoint_info_`
    auto conn = std::make_unique<MysqlExecutor>("localhost", "root", "abc123", "test");
    conn->QueryAll(res, sql_str, args...);

    p.SetValue(std::move(res));
    // p.SetException()
  });

  return fu.Then([context, this](Future<MysqlResults<OutputArgs...>>&& fu) {
    if (fu.IsFailed()) {
      // RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
      return MakeExceptionFuture<MysqlResults<OutputArgs...>>(fu.GetException());
    }
    auto mysql_res = fu.GetValue0();
    // RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
    return MakeReadyFuture<MysqlResults<OutputArgs...>>(std::move(mysql_res));
  });
}

}  // namespace trpc::mysql
