#include "trpc/client/mysql/mysql_service_proxy.h"
#include <iostream>
#include "trpc/client/service_proxy_option.h"
#include "trpc/util/string/string_util.h"
namespace trpc::mysql {

MysqlServiceProxy::MysqlServiceProxy() {
  // need to get option from client config
  ::trpc::ThreadPoolOption thread_pool_option;
  thread_pool_option.thread_num = 8;
  thread_pool_option.bind_core = true;
  thread_pool_ = std::make_unique<::trpc::ThreadPool>(std::move(thread_pool_option));
  thread_pool_->Start();
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

  // pool_manager_ = std::make_unique<MysqlExecutorPoolManager>(std::move(pool_option));
  pool_manager_ = trpc::MakeRefCounted<MysqlExecutorPoolManager>(std::move(pool_option));
  //  pool_manager_inited_.store(true);
  return true;
}

void MysqlServiceProxy::Destroy() {
  ServiceProxy::Destroy();
  thread_pool_->Join();
}

void MysqlServiceProxy::Stop() {
  ServiceProxy::Stop();
  thread_pool_->Stop();
}

void MysqlServiceProxy::InitOtherMembers() { InitManager(); }

}  // namespace trpc::mysql