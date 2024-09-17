#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mysql_service_proxy.h"

#include "trpc/client/make_client_context.h"
#include "trpc/future/future_utility.h"

namespace trpc::testing {

/*
mysql> select * from users;
+----+----------+-------------------+---------------------+
| id | username | email             | created_at          |
+----+----------+-------------------+---------------------+
|  1 | alice    | alice@example.com | 2024-08-17 21:45:57 |
|  2 | bob      | bob@abc.com       | 2024-08-17 21:45:57 |
|  3 | carol    | carol@example.com | 2024-08-17 21:45:57 |
| 19 | rose     | NULL              | 2024-09-01 12:33:24 |
+----+----------+-------------------+---------------------+
*/

std::shared_ptr<ServiceProxyOption> option_ = std::make_shared<ServiceProxyOption>();

TEST(proxy, Query) {
  // trpc::mysql::MysqlServiceProxy proxy;
  auto proxy = std::make_shared<trpc::mysql::MysqlServiceProxy>();
  trpc::ClientContextPtr ptr(nullptr);
  trpc::mysql::MysqlResults<int, std::string> res;
  proxy->Query(ptr, res, "select id, username from users where id = ?", 1);
  auto& res_data = res.GetResultSet();
  EXPECT_EQ("alice", std::get<1>(res_data[0]));
}

TEST(proxy, AsyncQuery) {
  // trpc::mysql::MysqlServiceProxy proxy;
  auto proxy = std::make_shared<trpc::mysql::MysqlServiceProxy>();
  trpc::ClientContextPtr ptr(nullptr);
  auto res = proxy->AsyncQuery<int, std::string>(ptr, "select id, username from users where id = ?", 1)
                 .Then([](trpc::Future<trpc::mysql::MysqlResults<int, std::string>>&& f) {
                   if (f.IsReady()) {
                     std::vector<std::tuple<int, std::string>> res_data;
                     f.GetValue0().GetResultSet(res_data);
                     EXPECT_EQ("alice", std::get<1>(res_data[0]));
                   }
                   return trpc::MakeReadyFuture<>();
                 });

  ::trpc::future::BlockingGet(std::move(res));
}

}  // namespace trpc::testing