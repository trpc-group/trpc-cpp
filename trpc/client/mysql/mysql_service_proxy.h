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

  ~MysqlServiceProxy() override = default;

  template <typename... OutputArgs, typename... InputArgs>
  Status Query(const ClientContextPtr& context, MysqlResults<OutputArgs...>& res, const std::string& sql_str,
               const InputArgs&... args);

  template <typename... OutputArgs, typename... InputArgs>
  Future<MysqlResults<OutputArgs...>> AsyncQuery(const ClientContextPtr& context, const std::string& sql_str,
                                                 const InputArgs&... args);

  void Destroy();

  void ServiceOptionToMysqlConfig();

  bool SplitAddressPort(const std::string& address, std::string& ip, uint32_t& port);

  // pool_manager_ only can be init after the service option is set.
  bool InitManager();

  Status GetExecutorAndCheck(const ClientContextPtr& context, MysqlExecutor*& conn);

 protected:
  void SetServiceProxyOptionInner(const std::shared_ptr<ServiceProxyOption>& option) override;

 private:
  std::unique_ptr<ThreadPool> thread_pool_{nullptr};
  std::unique_ptr<MysqlExecutorPoolManager> pool_manager_;
  std::atomic<bool> pool_manager_inited_{false};
};

template <typename... OutputArgs, typename... InputArgs>
Status MysqlServiceProxy::Query(const ClientContextPtr& context, MysqlResults<OutputArgs...>& res,
                                const std::string& sql_str, const InputArgs&... args) {
  // Adaptive event primitive for both fiber and pthread context
  FiberEvent e;
  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_status == FilterStatus::REJECT) {
    TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
    const Status& result = context->GetStatus();
    res.SetErrorMessage(result.ErrorMessage());
  }

  thread_pool_->AddTask([this, &context, &e, &res, &sql_str, &args...]() {
    MysqlExecutor* conn = nullptr;
    Status status = GetExecutorAndCheck(context, conn);
    if (!status.OK()) {
      res.SetErrorMessage(status.ErrorMessage());
      e.Set();
      return;
    }
    conn->QueryAll(res, sql_str, args...);
    e.Set();
  });

  e.Wait();
  RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  return Status();
}

template <typename... OutputArgs, typename... InputArgs>
Future<MysqlResults<OutputArgs...>> MysqlServiceProxy::AsyncQuery(const ClientContextPtr& context,
                                                                  const std::string& sql_str,
                                                                  const InputArgs&... args) {
  Promise<MysqlResults<OutputArgs...>> pr;
  auto fu = pr.GetFuture();

  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_status == FilterStatus::REJECT) {
    TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
    context->SetRequestData(nullptr);
    const Status& result = context->GetStatus();
    auto exception_fut =
        MakeExceptionFuture<MysqlResults<OutputArgs...>>(CommonException(result.ErrorMessage().c_str()));
    filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
    return exception_fut;
  }

  thread_pool_->AddTask([p = std::move(pr), this, context, sql_str, args...]() mutable {
    MysqlResults<OutputArgs...> res;
    MysqlExecutor* conn = nullptr;
    Status status = GetExecutorAndCheck(context, conn);
    if (!status.OK()) {
      p.SetException(CommonException(status.ErrorMessage()));
      return;
    }
    conn->QueryAll(res, sql_str, args...);
    p.SetValue(std::move(res));
    // p.SetException()
  });

  return fu.Then([context, this](Future<MysqlResults<OutputArgs...>>&& fu) {
    if (fu.IsFailed()) {
      RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
      return MakeExceptionFuture<MysqlResults<OutputArgs...>>(fu.GetException());
    }
    auto mysql_res = fu.GetValue0();
    RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
    return MakeReadyFuture<MysqlResults<OutputArgs...>>(std::move(mysql_res));
  });
}

Status GetExecutorAndCheck(const ClientContextPtr& context, MysqlExecutor*& conn) {
  NodeAddr node_addr;
  node_addr.ip = context->GetIp();
  node_addr.port = context->GetPort();

  // 从连接池管理器中获取连接池
  auto pool = this->pool_manager_->Get(node_addr);
  if (!pool) {
    return Status(-1, "Failed to get MysqlExecutorPool for the given node address.");
  }

  // 从连接池中获取执行器
  conn = pool->GetExecutor();
  if (!conn) {
    return Status(-1, "Failed to get MysqlExecutor for the given node address.");
  }

  return Status();  // 正常返回，Status默认为成功状态（ret_和func_ret_都为0）
}

}  // namespace trpc::mysql
