#include "trpc/client/mysql/mysql_executor_pool_manager.h"
#include <chrono>
#include <iostream>
#include <thread>
#include "trpc/client/mysql/mysql_executor.h"
#include "trpc/common/config/mysql_client_conf.h"
#include "trpc/common/config/trpc_config.h"

#include "gtest/gtest.h"

using namespace std;
using namespace std::chrono;
using namespace trpc::mysql;

namespace trpc {
namespace testing {

void clearPersonTable() {
  MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
  char sql[1024] = {0};
  sprintf(sql, "DELETE FROM person");
  MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Execute(res, sql);
}

MysqlClientConf readMysqlClientConf() {
  trpc::TrpcConfig* trpc_config = trpc::TrpcConfig::GetInstance();
  trpc_config->Init("./trpc/client/mysql/testing/fiber.yaml");
  const auto& client = trpc_config->GetClientConfig();
  return client.service_proxy_config[0].mysql_conf;
}

void op1(int begin, int end) {
  for (int i = begin; i < end; i++) {
    MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
    char sql[1024] = {0};
    sprintf(sql, "insert into person values(%d, 18, 'man', 'starry')", i);
    MysqlResults<trpc::mysql::OnlyExec> res;
    conn.Execute(res, sql);
  }
}

void op2(MysqlExecutorPoolManager* manager, const MysqlClientConf& conf, int begin, int end) {
  for (int i = begin; i < end; i++) {
    auto pool = manager->getPool(conf);  // Get the pool with conf parameter
    shared_ptr<MysqlExecutor> conn = pool->getConnection();
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
  MysqlClientConf conf = readMysqlClientConf();
  MysqlExecutorPoolManager manager;  // 使用构造函数创建 manager 实例
  steady_clock::time_point begin = steady_clock::now();
  op2(&manager, conf, 0, 500);  // 传递 manager 和 conf
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
  MysqlClientConf conf = readMysqlClientConf();
  MysqlExecutorPoolManager manager;  // 使用构造函数创建 manager 实例
  steady_clock::time_point begin = steady_clock::now();
  thread t1(op2, &manager, conf, 0, 100);
  thread t2(op2, &manager, conf, 100, 200);
  thread t3(op2, &manager, conf, 200, 300);
  thread t4(op2, &manager, conf, 300, 400);
  thread t5(op2, &manager, conf, 400, 500);
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
