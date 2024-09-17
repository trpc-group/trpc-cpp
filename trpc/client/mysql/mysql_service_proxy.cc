#include "trpc/client/mysql/mysql_service_proxy.h"
#include "mysql_service_proxy.h"
#include "trpc/common/config/mysql_client_conf.h"
namespace trpc::mysql {

MysqlServiceProxy::MysqlServiceProxy() {
  // need to get option from client config
  ::trpc::ThreadPoolOption thread_pool_option;
  thread_pool_option.thread_num = 2;
  thread_pool_option.bind_core = true;
  thread_pool_ = std::make_unique<::trpc::ThreadPool>(std::move(thread_pool_option));
  thread_pool_->Start();

  pool_manager_ = std::make_unique<MysqlExecutorPoolManager>();
}

}  // namespace trpc::mysql