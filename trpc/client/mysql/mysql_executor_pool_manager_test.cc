#include "trpc/client/mysql/mysql_executor_pool_manager.h"
#include <chrono>
#include <iostream>
#include <thread>
#include "trpc/client/mysql/executor/mysql_executor.h"
#include "trpc/client/mysql/mysql_executor_pool.h"
#include "trpc/client/mysql/mysql_service_config.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/transport/common/transport_message_common.h"

#include "gtest/gtest.h"

using namespace std;
using namespace std::chrono;
using namespace trpc::mysql;
using MysqlExecutorPoolOption = trpc::mysql::MysqlExecutorPoolOption;

namespace trpc {
namespace testing {

void clearPersonTable() {
  MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
  char sql[1024] = {0};
  sprintf(sql, "DELETE FROM person");
  MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Execute(res, sql);
}

trpc::mysql::MysqlExecutorPoolOption* readMysqlClientConf() {
  trpc::TrpcConfig* trpc_config = trpc::TrpcConfig::GetInstance();
  trpc_config->Init("./trpc/client/mysql/testing/fiber.yaml");
  const auto& client = trpc_config->GetClientConfig();
  trpc::mysql::MysqlExecutorPoolOption* conf = new trpc::mysql::MysqlExecutorPoolOption;
  conf->user_name = client.service_proxy_config[0].mysql_conf.user_name;
  conf->password = client.service_proxy_config[0].mysql_conf.password;
  conf->dbname = client.service_proxy_config[0].mysql_conf.dbname;
  conf->min_size = client.service_proxy_config[0].mysql_conf.min_size;
  conf->max_size = client.service_proxy_config[0].max_conn_num;
  conf->max_idle_time = client.service_proxy_config[0].idle_time;
  conf->timeout = client.service_proxy_config[0].timeout;
  std::string target = client.service_proxy_config[0].target;
  std::size_t pos = target.find(':');
  std::string ip = target.substr(0, pos);
  std::string port_str = target.substr(pos + 1);
  uint32_t port = static_cast<uint32_t>(std::stoul(port_str));
  return conf;
}

trpc::mysql::MysqlExecutorPool* initManager() {
  trpc::TrpcConfig* trpc_config = trpc::TrpcConfig::GetInstance();
  trpc_config->Init("./trpc/client/mysql/testing/fiber.yaml");
  const auto& client = trpc_config->GetClientConfig();

  trpc::mysql::MysqlExecutorPoolOption* conf = trpc::testing::readMysqlClientConf();
  trpc::mysql::MysqlExecutorPoolManager manager(std::move(*conf));
  std::string target = client.service_proxy_config[0].target;
  NodeAddr addr;
  std::size_t pos = target.find(':');
  addr.ip = target.substr(0, pos);
  std::string port_str = target.substr(pos + 1);
  addr.port = static_cast<uint16_t>(std::stoul(port_str));
  MysqlExecutorPool* pool = manager.Get(addr);
  return pool;
}

void op1(int begin, int end) {
  for (int i = begin; i < end; i++) {
    MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
    char sql[1024] = {0};
    sprintf(sql, "insert into person values(%d, 18, 'man', 'starry')", i);
    MysqlResults<trpc::mysql::OnlyExec> res;
    conn.Execute(res, sql);
    conn.Close();
  }
}

void op2(MysqlExecutorPool* pool, int begin, int end) {
  for (int i = begin; i < end; i++) {
    auto conn = pool->GetExecutor();
    char sql[1024] = {0};
    sprintf(sql, "insert into person values(%d, 18, 'man', 'starry')", i);
    MysqlResults<trpc::mysql::OnlyExec> res;
    conn->Execute(res, sql);
  }
}

// 性能测试
TEST(PerformanceTest, SingleThreadedWithoutPool) {
  clearPersonTable();  // 清空 person 表数据
  steady_clock::time_point begin = steady_clock::now();
  op1(0, 500);
  steady_clock::time_point end = steady_clock::now();
  auto length = end - begin;
  cout << "非连接池，单线程，用时：" << length.count() << "纳秒，" << length.count() / 1000000 << "毫秒" << endl;
}

TEST(PerformanceTest, SingleThreadedWithPool) {
  clearPersonTable();  // 清空 person 表数据
  MysqlExecutorPool* pool = initManager();
  steady_clock::time_point begin = steady_clock::now();
  op2(pool, 0, 500);
  steady_clock::time_point end = steady_clock::now();
  auto length = end - begin;
  cout << "连接池，单线程，用时：" << length.count() << "纳秒，" << length.count() / 1000000 << "毫秒" << endl;
}

TEST(PerformanceTest, MultiThreadedWithoutPool) {
  clearPersonTable();  // 清空 person 表数据
  steady_clock::time_point begin = steady_clock::now();
  thread t1(op1, 0, 100);
  thread t2(op1, 100, 200);
  thread t3(op1, 200, 300);
  thread t4(op1, 300, 400);
  thread t5(op1, 400, 500);
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();
  steady_clock::time_point end = steady_clock::now();
  auto length = end - begin;
  cout << "非连接池，多线程，用时：" << length.count() << "纳秒，" << length.count() / 1000000 << "毫秒" << endl;
}

TEST(PerformanceTest, MultiThreadedWithPool) {
  clearPersonTable();  // 清空 person 表数据
  MysqlExecutorPool* pool = initManager();
  steady_clock::time_point begin = steady_clock::now();
  thread t1(op2, pool, 0, 100);
  thread t2(op2, pool, 100, 200);
  thread t3(op2, pool, 200, 300);
  thread t4(op2, pool, 300, 400);
  thread t5(op2, pool, 400, 500);
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();
  steady_clock::time_point end = steady_clock::now();
  auto length = end - begin;
  cout << "连接池，多线程，用时：" << length.count() << "纳秒，" << length.count() / 1000000 << "毫秒" << endl;
}

}  // namespace testing
}  // namespace trpc
