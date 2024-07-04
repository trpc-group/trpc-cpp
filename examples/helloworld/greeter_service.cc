//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "examples/helloworld/greeter_service.h"

#include <string>

#include "trpc/common/status.h"
#include "trpc/log/trpc_log.h"

namespace test {
namespace helloworld {

::trpc::Status GreeterServiceImpl::SayHello(::trpc::ServerContextPtr context,
                                            const ::trpc::test::helloworld::HelloRequest* request,
                                            ::trpc::test::helloworld::HelloReply* reply) {
  // Your can access more information from rpc context, eg: remote ip and port
	TRPC_FMT_INFO("remote address: {}:{}", context->GetIp(), context->GetPort());
	TRPC_FMT_INFO("request name: {}", request->name());
	TRPC_FMT_INFO("request age: {}", request->age());
	std::string hobbies_str; 
	for (const auto& hobby : request->hobby()) {
		if (!hobbies_str.empty()) {
			hobbies_str += ", "; 
		}
		hobbies_str += hobby; 
	}
	TRPC_FMT_INFO("request hobby: {}", hobbies_str);
	
	reply->set_name("hello " + request->name());
	reply->set_age(request->age() + 1);
	for (const auto& hobby : request->hobby()) {
		reply->add_hobby(hobby);
	}
	
	return ::trpc::kSuccStatus;
}

}  // namespace helloworld
}  // namespace test
