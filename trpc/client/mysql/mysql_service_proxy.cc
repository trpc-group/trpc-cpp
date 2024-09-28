#include "trpc/client/mysql/mysql_service_proxy.h"
#include <iostream>
#include "mysql_service_proxy.h"
#include "trpc/client/service_proxy_option.h"
#include "trpc/common/config/mysql_client_conf.h"
#include "trpc/util/string/string_util.h"
namespace trpc::mysql {

MysqlServiceProxy::MysqlServiceProxy() {
  // need to get option from client config
  ::trpc::ThreadPoolOption thread_pool_option;
  thread_pool_option.thread_num = 2;
  thread_pool_option.bind_core = true;
  thread_pool_ = std::make_unique<::trpc::ThreadPool>(std::move(thread_pool_option));
  thread_pool_->Start();
}

//void MysqlServiceProxy::ServiceOptionToMysqlConfig() {
//  const ServiceProxyOption* option = GetServiceProxyOption();
//  std::vector<std::string> vec = util::SplitString(option->target, ',');
//  for (const auto& address : vec) {
//    std::string ip;
//    uint32_t port = 0;
//    if (!SplitAddressPort(address, ip, port)) {
//      continue;
//    }
//    MysqlClientConf* config = new MysqlClientConf;
//    config->dbname = option->mysql_conf.dbname;
//    config->user_name = option->mysql_conf.user_name;
//    config->password = option->mysql_conf.password;
//    config->enable = option->mysql_conf.enable;
//    config->connectpool.min_size = option->mysql_conf.min_size;
//    config->connectpool.timeout = option->timeout;
//    config->connectpool.max_size = option->max_conn_num;
//    config->connectpool.max_idle_time = option->idle_time;
//    config->ip = ip;
//    config->port = port;
//    vec_.push_back(config);
//    std::cout << "执行" << std::endl;
//  }
//}

bool MysqlServiceProxy::InitManager() {
  const ServiceProxyOption* option = GetServiceProxyOption();
  MysqlExecutorPoolOption pool_option;
  pool_option.user_name = option->mysql_conf.user_name;
  pool_option.password = option->mysql_conf.password;
  pool_option.db_name = option->mysql_conf.dbname;
  pool_option.timeout = option->timeout;
  pool_option.min_size = option->mysql_conf.min_size;
  pool_option.max_size = option->mysql_conf.max_size;
  pool_option.max_idle_time = option->mysql_conf.max_idle_time;

  pool_manager_ = std::make_unique<MysqlExecutorPoolManager>(std::move(pool_option));
  pool_manager_inited_.store(true);
  return true;

}

bool MysqlServiceProxy::SplitAddressPort(const std::string& address, std::string& ip, uint32_t& port) {
  size_t pos = address.find(':');
  if (pos == std::string::npos) {
    std::cerr << "Invalid address format: " << address << std::endl;
    return false;
  }

  ip = address.substr(0, pos);
  try {
    port = std::stoi(address.substr(pos + 1));
  } catch (const std::exception& e) {
    std::cerr << "Failed to parse port number from address: " << address << std::endl;
    return false;
  }

  return true;
}

void MysqlServiceProxy::Destroy() {
  ServiceProxy::Destroy();
  thread_pool_->Stop();
  thread_pool_->Join();
}

void MysqlServiceProxy::SetServiceProxyOptionInner(const std::shared_ptr<ServiceProxyOption> &option) {
  ServiceProxy::SetServiceProxyOptionInner(option);
  InitManager();
}

}  // namespace trpc::mysql