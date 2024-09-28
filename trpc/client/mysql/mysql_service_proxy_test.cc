#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mysql_service_proxy.h"

#include "trpc/client/make_client_context.h"
#include "trpc/future/future_utility.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/client/service_proxy_option_setter.h"

namespace trpc::testing {


 class MockMysqlServiceProxy : public mysql::MysqlServiceProxy {
  public:
   void SetMockServiceProxyOption(const std::shared_ptr<ServiceProxyOption>& option) {
     SetServiceProxyOptionInner(option);
   }

   void SetMockCodec(ClientCodecPtr && codec) {codec_ = codec; }

   void PrintResultTable(const mysql::MysqlResults<mysql::IterMode>& res) {
     std::vector<std::string> fields_name;
     bool flag = false;
     std::vector<size_t> column_widths;
     for(auto row : res) {
       if(!flag) {
         fields_name = row.GetFieldsName();
         column_widths.resize(fields_name.size(), 0);
         for (size_t i = 0; i < fields_name.size(); ++i)
           column_widths[i] = std::max(column_widths[i], fields_name[i].length());
         flag = true;
       }

       size_t i = 0;
       for(auto field : row) {
         column_widths[i] = std::max(column_widths[i], field.length());
         ++i;
       }
     }


     for (size_t i = 0; i < fields_name.size(); ++i) {
       std::cout << std::left << std::setw(column_widths[i] + 2) << fields_name[i];
     }
     std::cout << std::endl;

     for (size_t i = 0; i < fields_name.size(); ++i) {
       std::cout << std::setw(column_widths[i] + 2) << std::setfill('-') << "";
     }
     std::cout << std::endl;
     std::cout << std::setfill(' ');

     for (auto row : res) {
       int i = 0;
       for(auto field_itr = row.begin(); field_itr != row.end(); ++field_itr) {
         std::cout << std::left
                   << std::setw(column_widths[i] + 2)
                   << (field_itr.IsNull() ? "null" : *field_itr);
         ++i;
       }
       std::cout << std::endl;
     }
   }
 };
using MysqlServiceProxyPtr = std::shared_ptr<MockMysqlServiceProxy>;

class MysqlServiceProxyTest : public ::testing::Test {
public:
 static void SetUpTestCase() {
   TrpcPlugin::GetInstance()->RegisterPlugins();
   trpc::detail::SetDefaultOption(option_);
   option_->name = "default_mysql_service";
   option_->caller_name = "";
   option_->codec_name = "mysql";
   option_->conn_type = "long";
   option_->network = "tcp";
   option_->timeout = 1000;
   option_->target = "127.0.0.1:3306";
   option_->selector_name = "direct";
   option_->mysql_conf.dbname = "test";
   option_->mysql_conf.password = "abc123";
   option_->mysql_conf.user_name = "root";
   option_->mysql_conf.enable = true;
   option_->mysql_conf.min_size = 6;
 }

 static void TearDownTestCase() { TrpcPlugin::GetInstance()->UnregisterPlugins(); }

protected:
  void SetUp() override {
   mock_mysql_service_proxy_ = std::make_shared<MockMysqlServiceProxy>();
   mock_mysql_service_proxy_->SetMockServiceProxyOption(option_);
 }

 void TearDown() {
   mock_mysql_service_proxy_->Stop();
   mock_mysql_service_proxy_->Destroy();
 }

 ClientContextPtr GetClientContext() {
   auto ctx = MakeClientContext(mock_mysql_service_proxy_);
   ctx->SetTimeout(option_->timeout);
   ctx->SetAddr("127.0.0.1", 3306);
   return ctx;
 }
protected:
 static std::shared_ptr<ServiceProxyOption> option_;
 MysqlServiceProxyPtr mock_mysql_service_proxy_{nullptr};
};

std::shared_ptr<ServiceProxyOption> MysqlServiceProxyTest::option_ = std::make_shared<ServiceProxyOption>();

/*
mysql> select * from users;
+----+----------+-------------------+---------------------+------------+
| id | username | email             | created_at          | meta       |
+----+----------+-------------------+---------------------+------------+
|  1 | alice    | alice@example.com | 2024-09-08 13:16:24 | NULL       |
|  2 | bob      | bob@abc.com       | 2024-09-08 13:16:24 | NULL       |
|  3 | carol    | carol@example.com | 2024-09-08 13:16:24 | NULL       |
|  4 | rose     | NULL              | 2024-09-08 13:16:53 | NULL       |
+----+----------+-------------------+---------------------+------------+
*/

TEST_F(MysqlServiceProxyTest, Query) {
  auto client_context = GetClientContext();
  trpc::mysql::MysqlResults<int, std::string> res;
  mock_mysql_service_proxy_->Query(client_context, res, "select id, username from users where id = ?", 1);
  auto& res_vec = res.GetResultSet();
  EXPECT_EQ(true, res.IsSuccess());
  EXPECT_EQ("alice", std::get<1>(res_vec[0]));
}



TEST_F(MysqlServiceProxyTest, AsyncQuery) {
  // trpc::mysql::MysqlServiceProxy proxy;
  auto client_context = GetClientContext();
  auto res = mock_mysql_service_proxy_->AsyncQuery<mysql::IterMode>(client_context, "select * from users where id >= ?", 1)
                 .Then([this](trpc::Future<mysql::MysqlResults<mysql::IterMode>>&& f) {
                   if (f.IsReady()) {
                     auto row_iter = f.GetConstValue().begin();
                     EXPECT_EQ(5, (*row_iter).NumFields());
                     this->mock_mysql_service_proxy_->PrintResultTable(f.GetConstValue());
                   }
                   return trpc::MakeReadyFuture<>();
                 });

  ::trpc::future::BlockingGet(std::move(res));
}

}  // namespace trpc::testing