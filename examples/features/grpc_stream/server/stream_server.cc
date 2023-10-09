/*
*
* Tencent is pleased to support the open source community by making
* tRPC available.
*
* Copyright (C) 2023 THL A29 Limited, a Tencent company.
* All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include <string>
#include <utility>

#include "gflags/gflags.h"

#include "trpc/common/logging/trpc_logging.h"
#include "trpc/common/trpc_app.h"

#include "examples/features/grpc_stream/server/stream_service.h"

DEFINE_string(dbpath, "route_guide_db.json", "");

namespace test {
namespace helloworld {

class HelloWorldStreamServer : public ::trpc::TrpcApp {
public:
 int Initialize() override {
   const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
   std::string service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "RouteGuide");
   TRPC_FMT_INFO("service name:{}", service_name);
   RegisterService(service_name, std::make_shared<::test::helloworld::RouteGuideImpl>(FLAGS_dbpath));

   return 0;
 }

 void Destroy() override {}
};

}  // namespace helloworld
}  // namespace test

int main(int argc, char** argv) {
 test::helloworld::HelloWorldStreamServer server;

 server.Main(argc, argv);
 server.Wait();

 return 0;
}
