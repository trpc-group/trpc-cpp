[中文](../zh/trpc_protobuf_json_noop_guide.md)

## Overview

tRPC-Cpp provides a variety of message serialization and deserialization methods to address a rich variety of business scenarios. This article introduces the message serialization features of the tRPC-Cpp framework (hereinafter referred to as tRPC), offering developers insights into the following:
- How to use `Protobuf Message` as the message type.
- How to use `JSON` as the message type.
- How to use binary data (`noop`) as the message type.

tRPC-Cpp framework by default uses `Protobuf Message` as the request and response message type, but also supports using the `trpc` protocol to transmit JSON and binary data messages. This means that JSON and binary data can be used as request and response message types.

The message passing in this document is in the following format: `{ name:"issueshooter", age:18, hobby:["opensource project","movies","books"]}` After receiving the information, the server modifies certain fields and returns all the data to the client.

## Implementation Process

### 1-tRPC-Cpp interaction using protobuf serialization and deserialization

#### 1.1-Update proto message definition
Open the `examples/helloworld/helloworld.proto` file and add fields like name, age, and hobby to the `Request` & `Reply` messages. Its definition is as follows:
```c++
syntax = "proto3";

package trpc.test.helloworld;

service Greeter {
  rpc SayHello (HelloRequest) returns (HelloReply) {}
}

message HelloRequest {
  string name = 1;
  int32 age = 2;
  repeated string hobby = 3;
}

message HelloReply {
  string name = 1;
  int32 age = 2;
  repeated string hobby = 3;
}
```

#### 1.2-Update server program
###### 1.2.1-Open the `examples/helloworld/greeter_service.cc` file and modify the `Test` method's code implementation.
```c++
TEST(GreeterServiceTest, SayHelloOK) {
	auto server_context = MakeServerContext();
	::trpc::test::helloworld::HelloRequest request;
	request.set_name("issueshooter");
	request.set_age(18);
	request.add_hobby("opensource project");
	request.add_hobby("movies");
	request.add_hobby("books");
	
	::trpc::test::helloworld::HelloReply response;
	
	test::helloworld::GreeterServiceImpl svc_impl;
	::trpc::Status status = svc_impl.SayHello(server_context, &request, &response);
	
	ASSERT_TRUE(status.OK());
	//Verify that the server receives it correctly and adds 'hello' in front of it.
	ASSERT_EQ(response.name(), "hello issueshooter");
	//Verify that the server receives the data correctly and returns it with an increment.
	ASSERT_EQ(response.age(), 19); 
	ASSERT_EQ(response.hobby_size(), 3);
	ASSERT_EQ(response.hobby(0), "opensource project");
	ASSERT_EQ(response.hobby(1), "movies");
	ASSERT_EQ(response.hobby(2), "books");
}
```
###### 1.2.2-Open the `examples/helloworld/greeter_service_test.cc` file and modify the `SayHello` method's code implementation in `GreeterServiceImpl`.
```c++
::trpc::Status GreeterServiceImpl::SayHello(::trpc::ServerContextPtr context,const ::trpc::test::helloworld::HelloRequest* request,::trpc::test::helloworld::HelloReply* reply) {
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

	//Add 'hello' in front and send it back.
	reply->set_name("hello " + request->name());
	//Add one to age and send it back。
	reply->set_age(request->age() + 1);
	for (const auto& hobby : request->hobby()) {
		reply->add_hobby(hobby);
	}
	
	return ::trpc::kSuccStatus;
}
```

#### 1.3-Update client program
###### 1.3.1-Open the `examples/helloworld/test/fiber_client.cc` file and modify the `DoRpcCall` method's code.
```c++
int DoRpcCall(const std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>& proxy) {
	::trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy);
	
	::trpc::test::helloworld::HelloRequest req;
	req.set_name("issueshooter");
	req.set_age(18);
	req.add_hobby("opensource project");
	req.add_hobby("movies");
	req.add_hobby("books");
	
	::trpc::test::helloworld::HelloReply rsp;
	::trpc::Status status = proxy->SayHello(client_ctx, req, &rsp);
	if (!status.OK()) {
		std::cerr << "get rpc error: " << status.ErrorMessage() << std::endl;
		return -1;
	}
	std::cout << "get rsp: \n name: " << rsp.name() << std::endl;
	std::cout << " age: " << rsp.age() << std::endl;
	std::cout << " hobby: ";
	std::string hobbies_str;
	for (const auto& hobby : rsp.hobby()) {
		if (!hobbies_str.empty()) {
			hobbies_str += ", "; 
		}
		hobbies_str += hobby; 
	}
	std::cout << hobbies_str << std::endl;
	return 0;
}
```
###### 1.3.2-Open the `examples/helloworld/test/future_client.cc` file and modify the `DoAsyncRpcCall` method as shown in the following code.
```c++
int DoAsyncRpcCall(const std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>& proxy) {
	::trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy);
	
	::trpc::test::helloworld::HelloRequest req;
	req.set_name("issueshooter");
	req.set_age(18);
	req.add_hobby("opensource project");
	req.add_hobby("movies");
	req.add_hobby("books");
	
	bool succ = true;
	auto fut = proxy->AsyncSayHello(client_ctx, req)
		.Then([&succ](::trpc::Future<::trpc::test::helloworld::HelloReply>&& fut) {
			if (fut.IsReady()) {
				auto rsp = fut.GetValue0();
				std::cout << "get rsp:" << std::endl;
				std::cout << " name: " << rsp.name() << std::endl;
				std::cout << " age: " << rsp.age() << std::endl;
				std::cout << " hobby: ";
				std::string hobbies_str;
				for (const auto& hobby : rsp.hobby()) {
					if (!hobbies_str.empty()) {
						hobbies_str += ", ";
					}
					hobbies_str += hobby;
				}
				std::cout << hobbies_str << std::endl;
			} else {
				auto exception = fut.GetException();
				succ = false;
				std::cerr << "get rpc error: " << exception.what() << std::endl;
			}
			return ::trpc::MakeReadyFuture<>();
		});
	::trpc::future::BlockingGet(std::move(fut));
	return succ ? 0 : -1;
}
```

#### 1.4-Run
Compile the updated HelloWorld code:
```
bazel build //examples/helloworld/...
```
Run the updated HelloWorld server:
```
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
```
Open another terminal and run the client to send RPC requests to server:
```
./bazel-bin/examples/helloworld/test/fiber_client --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml

Or

./bazel-bin/examples/helloworld/test/future_client --client_config=./examples/helloworld/test/conf/trpc_cpp_future.yaml
```
After client done, you will see the following output in client's terminal:
```
FLAGS_service_name:trpc.test.helloworld.Greeter
FLAGS_client_config:./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
get rsp: 
 name: hello issueshooter
 age: 19
 hobby: opensource project, movies, books

Or

FLAGS_service_name:trpc.test.helloworld.Greeter
FLAGS_client_config:./examples/helloworld/test/conf/trpc_cpp_future.yaml
get rsp:
 name: hello issueshooter
 age: 19
 hobby: opensource project, movies, books
```
Use the `cat helloworld_fiber.log` command to view the server's log information, which shows the server received the following client information:
```
request name: issueshooter
request age: 18
request hobby: opensource project, movies, books
```


### 2-tRPC-Cpp interaction using json serialization and deserialization
#### 2.1-Open the `examples/features/trpc_json/client/client.cc` file and modify the `Run` method based on the existing code, as follows.
```c++
int Run() {
	::trpc::ServiceProxyOption option;
	option.name = FLAGS_target;
	option.codec_name = "trpc";
	option.network = "tcp";
	option.conn_type = "long";
	option.timeout = 1000;
	option.selector_name = "direct";
	option.target = FLAGS_addr;
	
	auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::RpcServiceProxy>(FLAGS_target, option);
	
	std::string req_json_str = "{\"name\":\"issueshooter\",\"age\":18,\"hobby\":[\"opensource project\",\"movies\",\"books\"]}";
	
	rapidjson::Document hello_req;
	hello_req.Parse(req_json_str.c_str());
	
	if (hello_req.HasParseError()) {
		std::cout << "json parse error: " << hello_req.GetParseError() << ", msg: " << rapidjson::GetParseError_En(hello_req.GetParseError()) << std::endl;
		return -1;
	}
	
	rapidjson::Document hello_rsp;
	::trpc::ClientContextPtr context = ::trpc::MakeClientContext(proxy);
	context->SetTimeout(1000);
	context->SetFuncName("JsonSayHello");
	
	::trpc::Status status = proxy->UnaryInvoke<rapidjson::Document, rapidjson::Document>(context, hello_req, &hello_rsp);
	
	if (!status.OK()) {
		std::cout << "invoke error: " << status.ErrorMessage() << std::endl;
		return -1;
	}
	
	for (rapidjson::Value::ConstMemberIterator iter = hello_rsp.MemberBegin(); iter != hello_rsp.MemberEnd(); ++iter) {
		std::cout << "json name: " << iter->name.GetString() << ", value: ";
		if (iter->value.IsInt()) {
			std::cout << iter->value.GetInt() << std::endl;
		} else if (iter->value.IsString()){
			std::cout << iter->value.GetString() << std::endl;
		}else if (iter->value.IsArray()){
			std::string array_values;
			for (auto& v : iter->value.GetArray()){
				if (!array_values.empty()){
					array_values += ",";
				}
				array_values += v.GetString();
			}
			std::cout << array_values << std::endl;
		}
	}
	
	return 0;
}
```
#### 2.2-Open the `examples/features/trpc_json/server/demo_server.cc` file and modify the `DemoServiceImpl`, as follows.
```c++
class DemoServiceImpl : public ::trpc::RpcServiceImpl {
public:
	DemoServiceImpl() {
		auto handler = new ::trpc::RpcMethodHandler<rapidjson::Document, rapidjson::Document>(std::bind(&DemoServiceImpl::JsonSayHello, this, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3));
		AddRpcServiceMethod(
	new ::trpc::RpcServiceMethod("JsonSayHello", ::trpc::MethodType::UNARY,handler));
	}

::trpc::Status JsonSayHello(const ::trpc::ServerContextPtr& context,
	const rapidjson::Document* request,
	rapidjson::Document* reply) {
		for (rapidjson::Value::ConstMemberIterator iter = request->MemberBegin(); iter != request->MemberEnd(); ++iter) {
			if (iter->value.IsInt()) {
				TRPC_FMT_INFO("json name: {}, value: {}", iter->name.GetString(), iter->value.GetInt());
			} else if (iter->value.IsArray()) {
				std::string array_values;
				for (auto& v : iter->value.GetArray()) {
					if (!array_values.empty()){
						array_values += ",";
					}
					array_values += v.GetString();
				}
				TRPC_FMT_INFO("json name: {}, values: {}", iter->name.GetString(), array_values);
			} else if (iter->value.IsString()) {
				TRPC_FMT_INFO("json name: {}, value: {}", iter->name.GetString(), iter->value.GetString());
			}
		}
	
		// Copy request to reply to start with the same JSON
		reply->CopyFrom(*request, reply->GetAllocator());
		// Check and modify the "name" field if it exists
		if (reply->HasMember("name") && (*reply)["name"].IsString()) {
			std::string modifiedName = "hello " + std::string((*reply)["name"].GetString());
			(*reply)["name"].SetString(modifiedName.c_str(), modifiedName.length(), reply->GetAllocator());
		}
		// Check and increment the "age" field if it exists
		if (reply->HasMember("age") && (*reply)["age"].IsInt()) {
			int incrementedAge = (*reply)["age"].GetInt() + 1;
			(*reply)["age"].SetInt(incrementedAge);
		}
	
		return ::trpc::kSuccStatus;
	}
};
```

#### 2.3-Run
Compile the updated HelloWorld code:
```
bazel build //examples/features/trpc_json/...
```
Run the updated HelloWorld server:
```
./bazel-bin/examples/features/trpc_json/server/demo_server --config=./examples/features/trpc_json/server/trpc_cpp_fiber.yam
```
Open another terminal and run the client to send RPC requests to server:
```
./bazel-bin/examples/features/trpc_json/client/client --client_config=./examples/features/trpc_json/client/trpc_cpp_fiber.yaml
```
After client done, you will see the following output in client's terminal:
```
FLAGS_target:trpc.test.helloworld.demo_service
FLAGS_client_config:./examples/features/trpc_json/client/trpc_cpp_fiber.yaml
son name: name, value: hello issueshooter
json name: age, value: 19
json name: hobby, value: opensource project,movies,book
```


### 3-tRPC-Cpp interaction using noop serialization and deserialization
#### 3.1-Open the `examples/features/trpc_noop/client/client.cc` file and modify the `SendStringMsg` and `SendNoncontiguousBufferMsg` methods, as follows.
```c++
void SendStringMsg(const std::shared_ptr<::trpc::RpcServiceProxy>& proxy) {
	nlohmann::json json_req_msg = {
		{"name","issueshooter"},
		{"age",18},
		{"hobby",{"opensource project","movies","books"}}
	};
	
	//Convert JSON to string
	std::string req_msg = json_req_msg.dump();
	std::string rsp_msg;

	::trpc::ClientContextPtr context = ::trpc::MakeClientContext(proxy);
	context->SetTimeout(1000);
	context->SetFuncName("NoopSayHello1");

	::trpc::Status status = proxy->UnaryInvoke<std::string, std::string>(context, req_msg, &rsp_msg);
	if (!status.OK()) {
		std::cout << "invoke error: " << status.ErrorMessage() << std::endl;
		return;
	}
	
	std::cout << "response msg: " << rsp_msg << std::endl;
}


void SendNoncontiguousBufferMsg(const std::shared_ptr<::trpc::RpcServiceProxy>& proxy) {
	nlohmann::json json_req_msg = {
		{"name","issueshooter"},
		{"age",18},
		{"hobby",{"opensource project","movies","books"}}
	};

	::trpc::NoncontiguousBuffer req_msg = ::trpc::CreateBufferSlow(json_req_msg.dump());
	::trpc::NoncontiguousBuffer rsp_msg;

	::trpc::ClientContextPtr context = ::trpc::MakeClientContext(proxy);
	context->SetTimeout(1000);
	context->SetFuncName("NoopSayHello2");
  
	::trpc::Status status =
proxy->UnaryInvoke<::trpc::NoncontiguousBuffer, ::trpc::NoncontiguousBuffer>(context, req_msg, &rsp_msg);
	if (!status.OK()) {
		std::cout << "invoke error: " << status.ErrorMessage() << std::endl;
		return;
	}

	std::cout << "response msg: " << ::trpc::FlattenSlow(rsp_msg) << std::endl;
}
```

#### 3.2-Open the `examples/features/trpc_noop/server/demo_server.cc `file and modify the `NoopSayHello1` and `NoopSayHello2` implementations, as follows.
```c++
::trpc::Status NoopSayHello1(const ::trpc::ServerContextPtr& context,
const std::string* request,
std::string* reply) {
	//Parse the JSON string back to JSON.
	nlohmann::json req_json = nlohmann::json::parse(*request);
	TRPC_FMT_INFO("request msg: {}", req_json.dump());
	
	if (req_json.contains("name") && req_json["name"].is_string()) {
		req_json["name"] = "hello " + req_json["name"].get<std::string>();
	}
	if (req_json.contains("age") && req_json["age"].is_number_integer()) {
		req_json["age"] = req_json["age"].get<int>() + 1;
	}

	//Convert modified JSON to string to send back.
	*reply = req_json.dump();
	return ::trpc::kSuccStatus;
}


::trpc::Status NoopSayHello2(const ::trpc::ServerContextPtr& context,
const ::trpc::NoncontiguousBuffer* request,
::trpc::NoncontiguousBuffer* reply) {
	nlohmann::json req_json = nlohmann::json::parse(::trpc::FlattenSlow(*request));
	TRPC_FMT_INFO("request msg: {}", req_json.dump());
	
	if (req_json.contains("age") && req_json["age"].is_number_integer()) {
		req_json["age"] = req_json["age"].get<int>() + 2; 
	}
	if (req_json.contains("name") && req_json["name"].is_string()) {
		req_json["name"] = "bye " + req_json["name"].get<std::string>();
	}
	
	std::string mod_req = req_json.dump();
	*reply = ::trpc::CreateBufferSlow(mod_req.data(), mod_req.size());
	return ::trpc::kSuccStatus;
}
```

#### 3.3-Run
Compile the updated HelloWorld code:
```
bazel build //examples/features/trpc_noop/...
```
Run the updated HelloWorld server:
```
./bazel-bin/examples/features/trpc_noop/server/demo_server --config=./examples/features/trpc_noop/server/trpc_cpp_fiber.yaml
```
Open another terminal and run the client to send RPC requests to server:
```
./bazel-bin/examples/features/trpc_noop/client/client --client_config=./examples/features/trpc_noop/client/trpc_cpp_fiber.yaml
```
After client done, you will see the following output in client's terminal:
```
FLAGS_target:trpc.test.helloworld.demo_service
FLAGS_client_config:./examples/features/trpc_noop/client/trpc_cpp_fiber.yaml
logging plugin configurations is not setted, log will be printed to the console.
response msg: {"age":19,"hobby":["opensource project","movies","books"],"name":"hello issueshooter"}
response msg: {"age":20,"hobby":["opensource project","movies","books"],"name":"bye issueshooter"}
```
