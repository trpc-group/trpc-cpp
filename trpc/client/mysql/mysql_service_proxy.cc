#include "trpc/client/mysql/mysql_service_proxy.h"
#include <iostream>
#include "trpc/client/service_proxy_option.h"
#include "trpc/util/string/string_util.h"
namespace trpc::mysql {

MysqlServiceProxy::MysqlServiceProxy() {

}

bool MysqlServiceProxy::InitManager() {
  const ServiceProxyOption* option = GetServiceProxyOption();
  MysqlExecutorPoolOption pool_option;
  pool_option.user_name = option->mysql_conf.user_name;
  pool_option.password = option->mysql_conf.password;
  pool_option.dbname = option->mysql_conf.dbname;
  pool_option.timeout = option->timeout;
  pool_option.min_size = option->mysql_conf.min_size;
  pool_option.max_size = option->mysql_conf.max_size;
  pool_option.max_idle_time = option->mysql_conf.max_idle_time;
  pool_option.num_shard_group = option->mysql_conf.num_shard_group;
  pool_option.use_back_thread_pool = option->mysql_conf.use_back_thread_pool;

  pool_manager_ = std::make_unique<MysqlExecutorPoolManager>(std::move(pool_option));
  // pool_manager_ = trpc::MakeRefCounted<MysqlExecutorPoolManager>(std::move(pool_option));
  //  pool_manager_inited_.store(true);
  return true;
}

bool MysqlServiceProxy::InitThreadPool() {
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

  if(handle.GetState() != TransactionHandle::TxState::kNotInited) {
    Status status = kUnknownErrorStatus;
    status.SetErrorMessage("The transaction handle is invalid or has been used.");
    context->SetStatus(status);
    return context->GetStatus();
  }


  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_status == FilterStatus::REJECT)
    TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
  else {
    NodeAddr node_addr;
    node_addr.ip = context->GetIp();
    node_addr.port = context->GetPort();
    MysqlExecutorPool* pool = this->pool_manager_->Get(node_addr);
    auto executor = pool->GetExecutor();
    if(executor == nullptr) {
      std::string error("");
      error += "Failed to get connection from pool: timeout while trying to acquire a connection.";
      error += node_addr.ip + ":" + std::to_string(node_addr.port);
      TRPC_LOG_ERROR(error);

      Status status;
      status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_CONNECT_ERR);
      status.SetErrorMessage(error);
      context->SetStatus(status);

      return context->GetStatus();
    }

    handle = TransactionHandle{std::move(executor)};
    handle.SetState(TransactionHandle::TxState::kStart);

    MysqlResults<OnlyExec> res;
    Execute(context, handle, res, "begin");

    if(!res.IsSuccess()) {
      Status status = kUnknownErrorStatus;
      status.SetErrorMessage(res.GetErrorMessage());
      context->SetStatus(status);
      handle.SetState(TransactionHandle::TxState::kNotInited);
      return context->GetStatus();
    }

  }
  return context->GetStatus();
}

Status MysqlServiceProxy::Commit(const ClientContextPtr &context, TransactionHandle &handle) {
  MysqlResults<OnlyExec> res;
  Status s = Execute(context, handle, res, "commit");

  if(!res.IsSuccess()) {
    Status status = kUnknownErrorStatus;
    status.SetErrorMessage(res.GetErrorMessage());
    context->SetStatus(status);
  } else {
    EndTransaction(handle);
  }

  return context->GetStatus();
}

Status MysqlServiceProxy::Rollback(const ClientContextPtr &context, TransactionHandle &handle) {
  MysqlResults<OnlyExec> res;
  Status s = Execute(context, handle, res, "rollback");

  if(!res.IsSuccess()) {
    Status status = kUnknownErrorStatus;
    status.SetErrorMessage(res.GetErrorMessage());
    context->SetStatus(status);
  } else {
    EndTransaction(handle);
  }

  return context->GetStatus();
}

bool MysqlServiceProxy::EndTransaction(TransactionHandle &handle) {
  handle.SetState(TransactionHandle::TxState::kEnd);
  NodeAddr node_addr;
  node_addr.ip = handle.GetExecutor()->GetIp();
  node_addr.port = handle.GetExecutor()->GetPort();
  MysqlExecutorPool* pool = pool_manager_->Get(node_addr);
  pool->Reclaim(0, handle.TransferExecutor());
  return true;
}


}  // namespace trpc::mysql