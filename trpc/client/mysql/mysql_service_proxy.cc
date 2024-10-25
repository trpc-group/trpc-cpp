#include "trpc/client/mysql/mysql_service_proxy.h"
#include <iostream>
#include "trpc/client/service_proxy_option.h"
#include "trpc/util/string/string_util.h"
namespace trpc::mysql {

MysqlServiceProxy::MysqlServiceProxy() {

}

bool MysqlServiceProxy::InitManager() {

  if(pool_manager_ != nullptr)
    return true;

  const ServiceProxyOption* option = GetServiceProxyOption();
  MysqlExecutorPoolOption pool_option;
  pool_option.user_name = option->mysql_conf.user_name;
  pool_option.password = option->mysql_conf.password;
  pool_option.dbname = option->mysql_conf.dbname;
  pool_option.max_size = option->max_conn_num;
  pool_option.max_idle_time = option->idle_time;
  pool_option.num_shard_group = option->mysql_conf.num_shard_group;
  pool_option.char_set = option->mysql_conf.char_set;
  pool_manager_ = std::make_unique<MysqlExecutorPoolManager>(std::move(pool_option));
  return true;
}

bool MysqlServiceProxy::InitThreadPool() {

  if(thread_pool_ != nullptr)
      return true;

  const ServiceProxyOption* option = GetServiceProxyOption();
  ::trpc::ThreadPoolOption thread_pool_option;
  thread_pool_option.thread_num = option->mysql_conf.thread_num;
  thread_pool_option.bind_core = option->mysql_conf.thread_bind_core;
  thread_pool_ = std::make_unique<::trpc::ThreadPool>(std::move(thread_pool_option));
  thread_pool_->Start();
  return true;
}


void MysqlServiceProxy::Destroy() {
  ServiceProxy::Destroy();
  thread_pool_->Join();
  pool_manager_->Destroy();
}

void MysqlServiceProxy::Stop() {
  ServiceProxy::Stop();
  thread_pool_->Stop();
  pool_manager_->Stop();
}

void MysqlServiceProxy::InitOtherMembers() {
  InitManager();
  InitThreadPool();
}


Status MysqlServiceProxy::Begin(const ClientContextPtr& context, TransactionHandle &handle) {

  FillClientContext(context);
  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_status == FilterStatus::REJECT) {
    TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
    RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
    return context->GetStatus();
  }

  MysqlResults<OnlyExec> res;
  NodeAddr node_addr;

  // Bypass the selector to use or test the service_proxy independently
  // (since the selector might not be registered)
  if(context->GetIp().empty()) {
    ClientContextPtr temp_ctx = MakeRefCounted<ClientContext>(this->GetClientCodec());
    FillClientContext(temp_ctx);

    if (!SelectTarget(temp_ctx)) {
      TRPC_LOG_ERROR("select target failed: " << temp_ctx->GetStatus().ToString());
      context->SetStatus(temp_ctx->GetStatus());
      return context->GetStatus();
    }

    node_addr = temp_ctx->GetNodeAddr();

  } else {
    node_addr = context->GetNodeAddr();
  }

  MysqlExecutorPool* pool = this->pool_manager_->Get(node_addr);
  auto executor = pool->GetExecutor();

  Status status;
  if(executor == nullptr) {
    status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_CONNECT_ERR);
    status.SetErrorMessage("connection failed");
    TRPC_FMT_ERROR("service name:{}, connection failed", GetServiceName());
    context->SetStatus(std::move(status));
  } else {
    status = UnaryInvoke(context, executor, res, "begin");
  }


  if(context->GetStatus().OK()) {
    handle.SetExecutor(std::move(executor));
    handle.SetState(TransactionHandle::TxState::kStart);
  }

  RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  return context->GetStatus();
}


Future<TransactionHandle> MysqlServiceProxy::AsyncBegin(const ClientContextPtr &context) {
  FillClientContext(context);

  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_status == FilterStatus::REJECT) {
    TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
    context->SetRequestData(nullptr);
    const Status& result = context->GetStatus();
    auto exception_fut =
            MakeExceptionFuture<TransactionHandle>(CommonException(result.ErrorMessage().c_str()));
    filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
    return exception_fut;
  }

  MysqlResults<OnlyExec> res;
  NodeAddr node_addr;


  // Bypass the selector to use or test the service_proxy independently
  // (since the selector might not be registered)
  if(context->GetIp().empty()) {
    ClientContextPtr temp_ctx = MakeRefCounted<ClientContext>(this->GetClientCodec());
    FillClientContext(temp_ctx);

    if (!SelectTarget(temp_ctx)) {
      TRPC_LOG_ERROR("select target failed: " << temp_ctx->GetStatus().ToString());
      context->SetStatus(temp_ctx->GetStatus());
      return MakeExceptionFuture<TransactionHandle>(trpc::CommonException(temp_ctx->GetStatus().ToString().c_str()));
    }
    node_addr = temp_ctx->GetNodeAddr();

  } else {
    node_addr = context->GetNodeAddr();
  }

  MysqlExecutorPool* pool = this->pool_manager_->Get(node_addr);
  auto executor = pool->GetExecutor();
  if(executor == nullptr) {
    TRPC_FMT_ERROR("service name:{}, connection failed", GetServiceName());
    Status status;
    status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_CONNECT_ERR);
    status.SetErrorMessage("connection failed");
    context->SetStatus(status);
    return MakeExceptionFuture<TransactionHandle>(CommonException("connection failed"));
  }

  return AsyncUnaryInvoke<OnlyExec>(context, executor, "begin")
          .Then([executor](Future<MysqlResults<OnlyExec>>&& f) mutable {
            TransactionHandle handle;
            if(f.IsFailed()) {
              return MakeExceptionFuture<TransactionHandle>(f.GetException());
            }
            handle.SetState(TransactionHandle::TxState::kStart);
            handle.SetExecutor(std::move(executor));
            return MakeReadyFuture(std::move(handle));
          });
}

Status MysqlServiceProxy::Commit(const ClientContextPtr &context, TransactionHandle &handle) {
  MysqlResults<OnlyExec> res;
  Status s = Execute(context, handle, res, "commit");

  if(!res.OK()) {
    Status status = kUnknownErrorStatus;
    status.SetErrorMessage(res.GetErrorMessage());
    context->SetStatus(status);
  } else {
    EndTransaction(handle, false);
  }

  return context->GetStatus();
}

Status MysqlServiceProxy::Rollback(const ClientContextPtr &context, TransactionHandle &handle) {
  MysqlResults<OnlyExec> res;
  Status s = Execute(context, handle, res, "rollback");

  if(!res.OK()) {
    Status status = kUnknownErrorStatus;
    status.SetErrorMessage(res.GetErrorMessage());
    context->SetStatus(status);
  } else {
    EndTransaction(handle, true);
  }

  return context->GetStatus();
}


Future<TransactionHandle> MysqlServiceProxy::AsyncCommit(const ClientContextPtr &context, TransactionHandle&& handle) {
  MysqlResults<OnlyExec> res;
  return AsyncQuery<OnlyExec>(context, std::move(handle), "commit")
          .Then([this, context](Future<TransactionHandle, MysqlResults<OnlyExec>>&& f){
            if(f.IsFailed()) {
              auto t = f.GetValue();
              return MakeExceptionFuture<TransactionHandle>(f.GetException());
            }
            auto t = f.GetValue();
            EndTransaction(std::get<0>(t), false);
            return MakeReadyFuture<TransactionHandle>(std::move(std::get<0>(t)));
          });
}

Future<TransactionHandle> MysqlServiceProxy::AsyncRollback(const ClientContextPtr &context, TransactionHandle&& handle) {
  MysqlResults<OnlyExec> res;
  return AsyncQuery<OnlyExec>(context, std::move(handle), "rollback")
          .Then([this, context](Future<TransactionHandle, MysqlResults<OnlyExec>>&& f){
            if(f.IsFailed()) {
              auto t = f.GetValue();
              return MakeExceptionFuture<TransactionHandle>(f.GetException());
            }
            auto t = f.GetValue();
            EndTransaction(std::get<0>(t), true);
            return MakeReadyFuture<TransactionHandle>(std::move(std::get<0>(t)));
          });
}


bool MysqlServiceProxy::EndTransaction(TransactionHandle &handle, bool rollback) {

  handle.SetState(rollback? TransactionHandle::TxState::kRollBacked : TransactionHandle::TxState::kCommitted);
  auto executor = handle.GetExecutor();
  if(executor) {
    NodeAddr node_addr;
    node_addr.ip = executor->GetIp();
    node_addr.port = executor->GetPort();
    MysqlExecutorPool *pool = pool_manager_->Get(node_addr);
    pool->Reclaim(0, handle.TransferExecutor());
  }
  return true;
}

}  // namespace trpc::mysql