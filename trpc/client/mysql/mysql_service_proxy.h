#pragma once

#include "trpc/client/mysql/executor/mysql_executor.h"
#include "trpc/client/mysql/mysql_executor_pool.h"
#include "trpc/client/mysql/mysql_executor_pool_manager.h"
#include "trpc/client/service_proxy.h"
#include "trpc/coroutine/fiber_event.h"
#include "trpc/util/ref_ptr.h"
#include "trpc/util/thread/latch.h"
#include "trpc/coroutine//fiber_latch.h"
#include "trpc/util/thread/thread_pool.h"
#include "trpc/client/mysql/transaction.h"

namespace trpc::mysql {

class MysqlServiceProxy : public ServiceProxy {

  using ExecutorPtr = RefPtr<MysqlExecutor>;
 public:
  MysqlServiceProxy();

  ~MysqlServiceProxy() override = default;

  /// @brief Executes a SQL query and retrieves all resulting rows.
  ///
  ///  This function executes the provided SQL query with the specified input arguments.
  ///
  /// @param context
  /// @param res Which return the query results. Details in "class MysqlResults"
  /// @param sql_str The SQL query to be executed as a string which uses "?" as
  ///  placeholders (see [MySQL C API Documentation](https://dev.mysql.com/doc/c-api/8.0/en/mysql-stmt-prepare.html)).
  /// @param args The input arguments to be bound to the query placeholders. Could be empty if no placeholders in sql_str.
  /// @return  A trpc Status. Note that the Status and the error information in MysqlResults are independent of each other.
  ///  If an error occurs during the MySQL query, the error will be stored in MysqlResults, but the Status will still be OK.
  ///  If no error occurs in the MySQL query but another error occurs in the call, MysqlResults will contain no error,
  ///  but the Status will reflect the error.
  template <typename... OutputArgs, typename... InputArgs>
  Status Query(const ClientContextPtr& context, MysqlResults<OutputArgs...>& res, const std::string& sql_str,
               const InputArgs&... args);


  /// @brief Async method for executing a SQL query and retrieves all resulting rows.
  ///
  ///  This function executes the provided SQL query with the specified input arguments.
  ///
  /// @param context
  /// @param res Which return the query results. Details in "class MysqlResults"
  /// @param sql_str The SQL query to be executed as a string which uses "?" as
  ///  placeholders (see [MySQL C API Documentation](https://dev.mysql.com/doc/c-api/8.0/en/mysql-stmt-prepare.html)).
  /// @param args The input arguments to be bound to the query placeholders. Could be empty if no placeholders in sql_str.
  /// @return  Future containing the MysqlResults. If an error occurs during the MySQL query, the error will be stored
  ///  in MysqlResults, and the exception future will also contain the same error. If no error occurs during the MySQL query
  ///  (e.g., timeout), there will be no error in MysqlResults, and you can retrieve the error from the exception future.
  template <typename... OutputArgs, typename... InputArgs>
  Future<MysqlResults<OutputArgs...>> AsyncQuery(const ClientContextPtr& context, const std::string& sql_str,
                                                 const InputArgs&... args);

  /// @brief "Execute" methods are completely the same with "Query" now.
  template <typename... OutputArgs, typename... InputArgs>
  Status Execute(const ClientContextPtr& context, MysqlResults<OutputArgs...>& res, const std::string& sql_str,
                 const InputArgs&... args);

  /// @brief "Execute" methods are completely the same with "Query" now.
  template <typename... OutputArgs, typename... InputArgs>
  Future<MysqlResults<OutputArgs...>> AsyncExecute(const ClientContextPtr& context, const std::string& sql_str,
                                                   const InputArgs&... args);


  /// @brief Transaction support for query. A TransactionHandle which has been called "Begin" is needed.
  template <typename... OutputArgs, typename... InputArgs>
  Status Query(const ClientContextPtr& context, TransactionHandle& handle, MysqlResults<OutputArgs...>& res, const std::string& sql_str,
               const InputArgs&... args);

  /// @brief Transaction support for query. A TransactionHandle which has been called "Begin" is needed.
  template <typename... OutputArgs, typename... InputArgs>
  Status Execute(const ClientContextPtr& context, TransactionHandle& handle, MysqlResults<OutputArgs...>& res, const std::string& sql_str,
                 const InputArgs&... args);

  template <typename... OutputArgs, typename... InputArgs>
  Future<TransactionHandle, MysqlResults<OutputArgs...>> AsyncQuery(const ClientContextPtr& context,
                                                                    TransactionHandle&& handle,
                                                                    const std::string& sql_str,
                                                                    const InputArgs&... args);

  template <typename... OutputArgs, typename... InputArgs>
  Future<TransactionHandle, MysqlResults<OutputArgs...>> AsyncExecute(const ClientContextPtr& context,
                                                                    TransactionHandle&& handle,
                                                                    const std::string& sql_str,
                                                                    const InputArgs&... args);

  /// @brief Begin a transaction. A empty handle is needed.
  Status Begin(const ClientContextPtr& context, TransactionHandle& handle);

  /// @brief Commit a transaction.
  Status Commit(const ClientContextPtr& context, TransactionHandle& handle);

  /// @brief Rollback a transaction.
  Status Rollback(const ClientContextPtr& context, TransactionHandle& handle);


  /// @brief Begin a transaction. A empty handle is needed.
  Future<TransactionHandle> AsyncBegin(const ClientContextPtr& context);

  /// @brief Commit a transaction.
  Future<TransactionHandle> AsyncCommit(const ClientContextPtr& context, TransactionHandle&& handle);

  /// @brief Rollback a transaction.
  Future<TransactionHandle> AsyncRollback(const ClientContextPtr& context, TransactionHandle&& handle);

  void Stop() override;

  void Destroy() override;

 protected:
  /// @brief Init pool_manager_.
  void InitOtherMembers() override;

 private:
  ///@brief pool_manager_ only can be inited after the service option has been set.
  bool InitManager();

  bool InitThreadPool();



   /// @param context
   /// @param executor If executor is nullptr, it will get a executor from executor manager.
   /// @param res
   /// @param sql_str
   /// @param args
   /// @return
  template <typename... OutputArgs, typename... InputArgs>
  Status UnaryInvoke(const ClientContextPtr& context, const ExecutorPtr& executor, MysqlResults<OutputArgs...>& res, const std::string& sql_str,
                     const InputArgs&... args);

  template <typename... OutputArgs, typename... InputArgs>
  Future<MysqlResults<OutputArgs...>> AsyncUnaryInvoke(const ClientContextPtr& context, const ExecutorPtr& executor, const std::string& sql_str,
                                                       const InputArgs&... args);

  bool EndTransaction(TransactionHandle& handle, bool rollback);

 private:
  std::unique_ptr<ThreadPool> thread_pool_{nullptr};
  std::unique_ptr<MysqlExecutorPoolManager> pool_manager_;
};

template <typename... OutputArgs, typename... InputArgs>
Status MysqlServiceProxy::Query(const ClientContextPtr& context, MysqlResults<OutputArgs...>& res,
                                const std::string& sql_str, const InputArgs&... args) {

  FillClientContext(context);

  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_status == FilterStatus::REJECT)
    TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
  else
    UnaryInvoke(context, nullptr, res, sql_str, args...);

  RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  return context->GetStatus();
}

template <typename... OutputArgs, typename... InputArgs>
Future<MysqlResults<OutputArgs...>> MysqlServiceProxy::AsyncQuery(const ClientContextPtr& context,
                                                                  const std::string& sql_str,
                                                                  const InputArgs&... args) {

  FillClientContext(context);

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

  return AsyncUnaryInvoke<OutputArgs...>(context, nullptr, sql_str, args...)
          .Then([this, context](Future<MysqlResults<OutputArgs...>>&& f){
            if(f.IsFailed()) {
              RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
              return MakeExceptionFuture<MysqlResults<OutputArgs...>>(f.GetException());
            }
            RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
            return MakeReadyFuture<MysqlResults<OutputArgs...>>(f.GetValue0());
          });
}

template <typename... OutputArgs, typename... InputArgs>
Status MysqlServiceProxy::Execute(const ClientContextPtr& context, MysqlResults<OutputArgs...>& res,
                                  const std::string& sql_str, const InputArgs&... args) {

    return Query(context, res, sql_str, args...);
}

template <typename... OutputArgs, typename... InputArgs>
Future<MysqlResults<OutputArgs...>> MysqlServiceProxy::AsyncExecute(const ClientContextPtr& context,
                                                                    const std::string& sql_str,
                                                                    const InputArgs&... args) {
    return AsyncQuery<OutputArgs...>(context, sql_str, args...);
}


template<typename... OutputArgs, typename... InputArgs>
Status MysqlServiceProxy::Query(const ClientContextPtr &context,
                         TransactionHandle &handle,
                         MysqlResults<OutputArgs...> &res,
                         const std::string &sql_str,
                         const InputArgs &... args) {

  FillClientContext(context);

  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_status == FilterStatus::REJECT)
    TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
  else if (handle.GetState() != TransactionHandle::TxState::kStart) {
    Status status;
    status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_CONNECT_ERR);
    status.SetErrorMessage("Invalid TransactionHandle state.");
    context->SetStatus(status);
  } else if(!handle.GetExecutor()->IsConnectionValid()) {
    handle.SetState(TransactionHandle::TxState::kRollBacked);
    Status status;
    status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_CONNECT_ERR);
    status.SetErrorMessage("Connect error. Rollback.");
    context->SetStatus(status);
  } else {
    UnaryInvoke(context, handle.GetExecutor(), res, sql_str, args...);
  }

  RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  return context->GetStatus();
}

template <typename... OutputArgs, typename... InputArgs>
Status MysqlServiceProxy::Execute(const ClientContextPtr& context,
                                  TransactionHandle& handle,
                                  MysqlResults<OutputArgs...>& res,
                                  const std::string& sql_str,
                                  const InputArgs&... args) {

    return Query(context, handle, res, sql_str, args...);
}



template<typename... OutputArgs, typename... InputArgs>
Future<TransactionHandle, MysqlResults<OutputArgs...>>
MysqlServiceProxy::AsyncQuery(const ClientContextPtr &context, TransactionHandle&& handle, const std::string &sql_str,
                              const InputArgs &... args) {

  FillClientContext(context);

  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_status == FilterStatus::REJECT) {
    TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
    context->SetRequestData(nullptr);
    const Status& result = context->GetStatus();
    auto exception_fut =
            MakeExceptionFuture<TransactionHandle, MysqlResults<OutputArgs...>>(CommonException(result.ErrorMessage().c_str()));
    filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
    return exception_fut;
  }

  if(!handle.GetExecutor()->IsConnectionValid()) {
    handle.SetState(TransactionHandle::TxState::kRollBacked);
    Status status;
    status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_CONNECT_ERR);
    status.SetErrorMessage("Connect error. Rollback.");
    context->SetStatus(status);
    auto exception_fut =
            MakeExceptionFuture<TransactionHandle, MysqlResults<OutputArgs...>>(CommonException("Connect error. Rollback."));
    filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
    return exception_fut;
  }

  auto executor = handle.GetExecutor();

  return AsyncUnaryInvoke<OutputArgs...>(context, executor, sql_str, args...)
          .Then([moved_handle = std::move(handle), this, context](Future<MysqlResults<OutputArgs...>>&& f) mutable{
            if(f.IsFailed()) {
              RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
              return MakeExceptionFuture<TransactionHandle, MysqlResults<OutputArgs...>>(f.GetException());
            }

            RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
            return MakeReadyFuture<TransactionHandle, MysqlResults<OutputArgs...>>(std::move(moved_handle), f.GetValue0());
          });
}

template <typename... OutputArgs, typename... InputArgs>
Future<TransactionHandle, MysqlResults<OutputArgs...>>
MysqlServiceProxy::AsyncExecute(const ClientContextPtr& context,
                                TransactionHandle&& handle,
                                const std::string& sql_str,
                                const InputArgs&... args) {

  return AsyncQuery<OutputArgs...>(context, std::move(handle), sql_str, args...);
}

template <typename... OutputArgs, typename... InputArgs>
Status MysqlServiceProxy::UnaryInvoke(const ClientContextPtr& context, const ExecutorPtr& executor, MysqlResults<OutputArgs...>& res,
                                      const std::string& sql_str, const InputArgs&... args) {

  if (CheckTimeout(context))
    return context->GetStatus();

  int filter_ret = RunFilters(FilterPoint::CLIENT_PRE_SEND_MSG, context);

  if(filter_ret == 0) {
    FiberEvent e;
    thread_pool_->AddTask([this, &context, &executor, &e, &res, &sql_str, &args...]() {
      ExecutorPtr conn{nullptr};
      MysqlExecutorPool* pool{nullptr};

      if(executor == nullptr) {
        NodeAddr node_addr;
        node_addr.ip = context->GetIp();
        node_addr.port = context->GetPort();
        pool = this->pool_manager_->Get(node_addr);
        conn = pool->GetExecutor();
      } else
        conn = executor;

      if(conn == nullptr) {
        Status status;
        status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_CONNECT_ERR);
        status.SetErrorMessage("connection failed");
        TRPC_FMT_ERROR("service name:{}, connection failed", GetServiceName());
        context->SetStatus(std::move(status));
      } else {
        if constexpr (MysqlResults<OutputArgs...>::mode == MysqlResultsMode::OnlyExec)
          conn->Execute(res, sql_str, args...);
        else
          conn->QueryAll(res, sql_str, args...);

        if(pool != nullptr)
          pool->Reclaim(0, std::move(conn));
      }

      e.Set();
    });

    e.Wait();
  }

  if(!res.OK()) {
    Status s;
    s.SetErrorMessage(res.GetErrorMessage());
    s.SetFuncRetCode(-1);
    context->SetStatus(std::move(s));
  }

  ProxyStatistics(context);
  RunFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);


  return context->GetStatus();
}

template <typename... OutputArgs, typename... InputArgs>
Future<MysqlResults<OutputArgs...>>
MysqlServiceProxy::AsyncUnaryInvoke(const ClientContextPtr& context, const ExecutorPtr& executor, const std::string& sql_str,
                                                                        const InputArgs&... args) {
  Promise<MysqlResults<OutputArgs...>> pr;
  auto fu = pr.GetFuture();

  if(CheckTimeout(context)) {
    const Status& result = context->GetStatus();
    return MakeExceptionFuture<MysqlResults<OutputArgs...>>(CommonException(result.ErrorMessage().c_str(), result.GetFrameworkRetCode()));
  }

  int filter_ret = RunFilters(FilterPoint::CLIENT_PRE_SEND_MSG, context);

  if(filter_ret != 0) {
    RunFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);
    RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
    return MakeExceptionFuture<MysqlResults<OutputArgs...>>(CommonException(context->GetStatus().ErrorMessage().c_str()));
  }


  thread_pool_->AddTask([p = std::move(pr), this, executor, context, sql_str, args...]() mutable {
    MysqlResults<OutputArgs...> res;
    NodeAddr node_addr;

    MysqlExecutorPool* pool{nullptr};
    ExecutorPtr conn{nullptr};


    if(executor == nullptr) {
      node_addr = context->GetNodeAddr();
      pool = this->pool_manager_->Get(node_addr);
      conn = pool->GetExecutor();
    } else
      conn = executor;

    if (TRPC_UNLIKELY(!conn)) {
      TRPC_FMT_ERROR("service name:{}, connection failed", GetServiceName());

      Status status;
      status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_CONNECT_ERR);
      status.SetErrorMessage("connection failed");

      context->SetStatus(status);
      p.SetException(CommonException(status.ErrorMessage().c_str()));
      return;
    }

    if constexpr (MysqlResults<OutputArgs...>::mode == MysqlResultsMode::OnlyExec)
      conn->Execute(res, sql_str, args...);
    else
      conn->QueryAll(res, sql_str, args...);

    if(pool != nullptr)
      pool->Reclaim(0, std::move(conn));

    ProxyStatistics(context);

    if (res.OK())
      p.SetValue(std::move(res));
    else
      p.SetException(CommonException(res.GetErrorMessage().c_str()));
  });

  return fu.Then([context, this](Future<MysqlResults<OutputArgs...>>&& fu) {
    if (fu.IsFailed()) {
      RunFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);
      return MakeExceptionFuture<MysqlResults<OutputArgs...>>(fu.GetException());
    }
    auto mysql_res = fu.GetValue0();
    RunFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);
    return MakeReadyFuture<MysqlResults<OutputArgs...>>(std::move(mysql_res));
  });
}

}  // namespace trpc::mysql
