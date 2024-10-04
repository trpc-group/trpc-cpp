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
  Future<MysqlResults<OutputArgs...>>
  AsyncQuery(const ClientContextPtr& context, const std::string& sql_str,
                                                 const InputArgs&... args);

  /// @brief Notice that this method can also be used for query data.
  template <typename... OutputArgs, typename... InputArgs>
  Status Execute(const ClientContextPtr& context, MysqlResults<OutputArgs...>& res, const std::string& sql_str,
               const InputArgs&... args);

  template <typename... OutputArgs, typename... InputArgs>
  Future<MysqlResults<OutputArgs...>>
  AsyncExecute(const ClientContextPtr& context, const std::string& sql_str,
                                                 const InputArgs&... args);


  void Stop() override;

  void Destroy() override;


 private:
  ///@brief pool_manager_ only can be inited after the service option has been set.
  bool InitManager();

  template <typename... OutputArgs, typename... InputArgs>
  Status UnaryInvoke(const ClientContextPtr& context, MysqlResults<OutputArgs...>& res, const std::string& sql_str,
                     const InputArgs&... args);


  template <typename... OutputArgs, typename... InputArgs>
  Future<MysqlResults<OutputArgs...>> AsyncUnaryInvoke(const ClientContextPtr& context, const std::string& sql_str,
                     const InputArgs&... args);



 protected:

  /// @brief Init pool_manager_.
  void InitOtherMembers() override;

 private:
  std::unique_ptr<ThreadPool> thread_pool_{nullptr};
  std::unique_ptr<MysqlExecutorPoolManager> pool_manager_;
};

template <typename... OutputArgs, typename... InputArgs>
Status MysqlServiceProxy::Query(const ClientContextPtr& context, MysqlResults<OutputArgs...>& res,
                                const std::string& sql_str, const InputArgs&... args) {

  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_status == FilterStatus::REJECT)
    TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
  else
    UnaryInvoke(context, res, sql_str, args...);

  RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  return context->GetStatus();
}

template <typename... OutputArgs, typename... InputArgs>
Future<MysqlResults<OutputArgs...>> MysqlServiceProxy::AsyncQuery(const ClientContextPtr& context,
                                                                  const std::string& sql_str,
                                                                  const InputArgs&... args) {

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

  return AsyncUnaryInvoke<OutputArgs...>(context, sql_str, args...);
}



template <typename... OutputArgs, typename... InputArgs>
Status MysqlServiceProxy::Execute(const ClientContextPtr& context, MysqlResults<OutputArgs...>& res, const std::string& sql_str,
               const InputArgs&... args) {

  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_status == FilterStatus::REJECT)
    TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
  else
    UnaryInvoke(context, res, sql_str, args...);

  RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  return context->GetStatus();
}

template <typename... OutputArgs, typename... InputArgs>
Future<MysqlResults<OutputArgs...>> MysqlServiceProxy::AsyncExecute(const ClientContextPtr& context, const std::string& sql_str,
                                                 const InputArgs&... args) {
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

  return AsyncUnaryInvoke<OutputArgs...>(context, sql_str, args...);
}


template<typename... OutputArgs, typename... InputArgs>
Status MysqlServiceProxy::UnaryInvoke(const ClientContextPtr &context,
                                      MysqlResults<OutputArgs...> &res,
                                      const std::string &sql_str,
                                      const InputArgs &... args) {

  FillClientContext(context);

  if(CheckTimeout(context))
      return context->GetStatus();

  RunFilters(FilterPoint::CLIENT_PRE_SEND_MSG, context);
  FiberEvent e;
  thread_pool_->AddTask([this, &context, &e, &res, &sql_str, &args...]() {

    NodeAddr node_addr;
    node_addr.ip = context->GetIp();
    node_addr.port = context->GetPort();
//    auto conn = this->pool_manager_->Get(node_addr)->GetExecutor();
    auto conn = std::make_unique<MysqlExecutor>("127.0.0.1", "root", "abc123", "test", 3306);

    if constexpr (MysqlResults<OutputArgs...>::mode == MysqlResultsMode::OnlyExec)
      conn->Execute(res, sql_str, args...);
    else
      conn->QueryAll(res, sql_str, args...);

//    sleep(1);

    e.Set();
  });

  e.Wait();
  RunFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);

  return context->GetStatus();
}


template <typename... OutputArgs, typename... InputArgs>
Future<MysqlResults<OutputArgs...>> MysqlServiceProxy::AsyncUnaryInvoke(const ClientContextPtr& context, const std::string& sql_str,
                                                     const InputArgs&... args) {
  FillClientContext(context);

  Promise<MysqlResults<OutputArgs...>> pr;
  auto fu = pr.GetFuture();

  RunFilters(FilterPoint::CLIENT_PRE_SEND_MSG, context);
  thread_pool_->AddTask([p = std::move(pr), this, context, sql_str, args...]() mutable {


    MysqlResults<OutputArgs...> res;
    NodeAddr node_addr;
    node_addr.ip = context->GetIp();
    node_addr.port = context->GetPort();
//    auto conn = this->pool_manager_->Get(node_addr)->GetExecutor();
    auto conn = std::make_unique<MysqlExecutor>("127.0.0.1", "root", "abc123", "test", 3306);

    if constexpr (MysqlResults<OutputArgs...>::mode == MysqlResultsMode::OnlyExec)
      conn->Execute(res, sql_str, args...);
    else
      conn->QueryAll(res, sql_str, args...);

//    sleep(1);


    RunFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);

    if(res.IsSuccess())
      p.SetValue(std::move(res));
    else
      p.SetException(CommonException(res.GetErrorMessage().c_str()));
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





}  // namespace trpc::mysql
