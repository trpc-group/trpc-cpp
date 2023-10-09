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

#include <string>

#include "fmt/format.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/compiler/plugin.pb.h"
#include "google/protobuf/descriptor.h"

#include "trpc/proto/trpc_options.pb.h"
#include "trpc/tools/comm/trpc_cpp_options.pb.h"
#include "trpc/tools/comm/utils.h"

using namespace trpc::tools;

static std::string GenHeader(const google::protobuf::FileDescriptor* file, const std::string& stub_path) {
  int indent = 0;
  std::string content;

  content += kDeclaration;

  std::string pbFileBaseName;
  if (stub_path.length() == 0) {
    pbFileBaseName = ProtoFileBaseName(file->name());
  } else {
    pbFileBaseName = stub_path;
  }

  content += fmt::format(R"(#include "{0}.trpc.pb.h")", pbFileBaseName);
  content += LineFeed(indent);
  content += LineFeed(indent);
  content += R"(#include "trpc/server/rpc_async_method_handler.h")";
  content += LineFeed(indent);
  content += R"(#include "trpc/server/rpc_method_handler.h")";
  content += LineFeed(indent);

  // If contains stream method, include specificed handler header.
  if (ContainStreamMethod(file)) {
    content += R"(#include "trpc/server/stream_rpc_async_method_handler.h")";
    content += LineFeed(indent);
    content += R"(#include "trpc/server/stream_rpc_method_handler.h")";
    content += LineFeed(indent);
  }

  return content;
}

/*
static const std::vector<std::vector<std::string_view>> Greeter_method_names = {
  {"alias-define", "muti-alias-define1", "muti-alias-define2", "/trpc.test.helloworld.Greeter/SayHello"},
};
*/
static std::string GenServiceMethodNameArray(const ::google::protobuf::ServiceDescriptor* service,
                                             const std::string& pkg, int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  out += LineFeed(indent);
  out += LineFeed(indent);
  out += fmt::format("static const std::vector<std::vector<std::string_view>> {0}_method_names = {{", service->name());

  for (int i = 0; i < service->method_count(); ++i) {
    out += LineFeed(indent + 1);

    out += "{";

    const auto& opts = service->method(i)->options();

    std::string alias_method = opts.GetExtension(trpc::alias);
    if (!alias_method.empty()) {
      out += fmt::format(R"("{0}", )", alias_method);
    }

    const trpc::CppExt cpp_ext = opts.GetExtension(trpc::cpp_ext);
    auto ext_alias_methods = cpp_ext.alias();
    for (const auto& ext_alias : ext_alias_methods) {
      out += fmt::format(R"("{0}", )", ext_alias);
    }

    out += fmt::format(R"("/{0}.{1}/{2}")", pkg, service->name(), service->method(i)->name());

    out += "},";
  }

  out += LineFeed(indent);
  out += "};";

  return out;
}

/*
Greeter::Greeter() {
  for (const std::string_view& method : Greeter_method_names[0]) {
    AddRpcServiceMethod(new trpc::RpcServiceMethod(method.data(), trpc::MethodType::UNARY, new
trpc::RpcMethodHandler<trpc::test::helloworld::HelloRequest,
trpc::test::helloworld::HelloReply>(std::bind(&Greeter::SayHello, this, std::placeholders::_1, std::placeholders::_2,
std::placeholders::_3))));
  }
}

Greeter::~Greeter() {}
*/
static std::string GenServiceConDestructor(const ::google::protobuf::ServiceDescriptor* service, const std::string& pkg,
                                           int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto& serviceName = service->name();

  out += LineFeed(indent);
  out += LineFeed(indent);

  // Generate service constructor
  out += serviceName + "::" + serviceName + "() {";
  for (int i = 0; i < service->method_count(); ++i) {
    auto method = service->method(i);

    out += LineFeed(indent + 1);

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();
    out +=
        fmt::format("for (const std::string_view& method : {0}_method_names[{1}]) {{", serviceName, std::to_string(i));
    out += LineFeed(indent + 2);
    if (!client_stream && !server_stream) {
      out += fmt::format(
          "AddRpcServiceMethod(new ::trpc::RpcServiceMethod(method.data(), ::trpc::MethodType::UNARY, new "
          "::trpc::RpcMethodHandler<{0}, {1}>(std::bind(&{2}::{3}, this, std::placeholders::_1, std::placeholders::_2, "
          "std::placeholders::_3))));",
          GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()), serviceName, method->name());
    } else if (client_stream && !server_stream) {
      out += fmt::format(
          "AddRpcServiceMethod(new ::trpc::RpcServiceMethod(method.data(), ::trpc::MethodType::CLIENT_STREAMING, new "
          "::trpc::StreamRpcMethodHandler<{0}, {1}>(std::bind(&{2}::{3}, this, std::placeholders::_1, "
          "std::placeholders::_2, std::placeholders::_3))));",
          GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()), serviceName, method->name());
    } else if (!client_stream && server_stream) {
      out += fmt::format(
          "AddRpcServiceMethod(new ::trpc::RpcServiceMethod(method.data(), ::trpc::MethodType::SERVER_STREAMING, new "
          "::trpc::StreamRpcMethodHandler<{0}, {1}>(std::bind(&{2}::{3}, this, std::placeholders::_1, "
          "std::placeholders::_2, std::placeholders::_3))));",
          GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()), serviceName, method->name());
    } else {
      out += fmt::format(
          "AddRpcServiceMethod(new ::trpc::RpcServiceMethod(method.data(), ::trpc::MethodType::BIDI_STREAMING, new "
          "::trpc::StreamRpcMethodHandler<{0}, {1}>(std::bind(&{2}::{3}, this, std::placeholders::_1, "
          "std::placeholders::_2, std::placeholders::_3))));",
          GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()), serviceName, method->name());
    }
    out += LineFeed(indent + 1);
    out += "}";
  }
  out += LineFeed(indent);
  out += "}";

  return out;
}

static std::string GenAsyncServiceConDestructor(const ::google::protobuf::ServiceDescriptor* service,
                                                const std::string& pkg, int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto serviceName = service->name();
  const auto nameSpace = GenNamespaceStr(pkg);
  const auto methodArrayName = serviceName + "_method_names";

  out += LineFeed(indent);
  out += LineFeed(indent);

  // Generate service constructor
  out += fmt::format(R"(Async{0}::Async{1}() : ::trpc::AsyncRpcServiceImpl() {{)", serviceName, serviceName);
  for (int i = 0; i < service->method_count(); ++i) {
    auto method = service->method(i);
    const auto input_type = GetParamterTypeWithNamespace(method->input_type()->full_name());
    const auto output_type = GetParamterTypeWithNamespace(method->output_type()->full_name());

    out += LineFeed(indent + 1);

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();

    out += fmt::format(R"(for (const std::string_view& method : {0}_method_names[{1}]) {{)", serviceName, i);
    out += LineFeed(indent + 2);
    if (!client_stream && !server_stream) {
      out += fmt::format(
          "AddRpcServiceMethod(new ::trpc::RpcServiceMethod(method.data(), ::trpc::MethodType::UNARY, new "
          "::trpc::AsyncRpcMethodHandler<{0}, {1}>(std::bind(&Async{2}::{3}, this, std::placeholders::_1, "
          "std::placeholders::_2))));",
          input_type, output_type, serviceName, method->name());
    } else if (client_stream && !server_stream) {
      out += fmt::format(
          "AddRpcServiceMethod(new ::trpc::RpcServiceMethod(method.data(), ::trpc::MethodType::CLIENT_STREAMING, new "
          "::trpc::AsyncStreamRpcMethodHandler<{0}, {1}>(std::bind(&Async{2}::{3}, this, std::placeholders::_1, "
          "std::placeholders::_2))));",
          input_type, output_type, serviceName, method->name());
    } else if (!client_stream && server_stream) {
      out += fmt::format(
          "AddRpcServiceMethod(new ::trpc::RpcServiceMethod(method.data(), ::trpc::MethodType::SERVER_STREAMING, new "
          "::trpc::AsyncStreamRpcMethodHandler<{0}, {1}>(std::bind(&Async{2}::{3}, this, std::placeholders::_1, "
          "std::placeholders::_2, std::placeholders::_3))));",
          input_type, output_type, serviceName, method->name());
    } else {
      out += fmt::format(
          "AddRpcServiceMethod(new ::trpc::RpcServiceMethod(method.data(), ::trpc::MethodType::BIDI_STREAMING, new "
          "::trpc::AsyncStreamRpcMethodHandler<{0}, {1}>(std::bind(&Async{2}::{3}, this, std::placeholders::_1, "
          "std::placeholders::_2))));",
          input_type, output_type, serviceName, method->name());
    }
    out += LineFeed(indent + 1);
    out += "}";
  }
  out += LineFeed(indent);
  out += "}";

  return out;
}

/*
::trpc::Status Greeter::SayHello(::trpc::ServerContextPtr context, const HelloRequest* request,
HelloReply* response) { (void)context; (void)request; (void)response; return ::trpc::Status(-1, "");
}
*/
static std::string GenServiceMethodImp(const ::google::protobuf::ServiceDescriptor* service, const std::string& pkg,
                                       int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto& serviceName = service->name();

  for (int i = 0; i < service->method_count(); ++i) {
    out += LineFeed(indent);
    out += LineFeed(indent);

    auto method = service->method(i);

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();
    if (!client_stream && !server_stream) {
      out += "::trpc::Status " + serviceName + "::" + method->name() + "(::trpc::ServerContextPtr context, const " +
             GetParamterTypeWithNamespace(method->input_type()->full_name()) + "* request, " +
             GetParamterTypeWithNamespace(method->output_type()->full_name()) + "* response) {";
      out += LineFeed(indent + 1) + "(void)context;";
      out += LineFeed(indent + 1) + "(void)request;";
      out += LineFeed(indent + 1) + "(void)response;";
      out += LineFeed(indent + 1) + R"(return ::trpc::Status(-1, "");)";
      out += LineFeed(indent);
      out += "}";
    } else if (client_stream && !server_stream) {
      out += "::trpc::Status " + serviceName + "::" + method->name() +
             "(const ::trpc::ServerContextPtr& context, const ::trpc::stream::StreamReader<" +
             GetParamterTypeWithNamespace(method->input_type()->full_name()) + ">& reader, " +
             GetParamterTypeWithNamespace(method->output_type()->full_name()) + "* response) {";
      out += LineFeed(indent + 1) + R"(return ::trpc::Status(-1, "");)";
      out += LineFeed(indent);
      out += "}";
    } else if (!client_stream && server_stream) {
      out +=
          "::trpc::Status " + serviceName + "::" + method->name() + "(const ::trpc::ServerContextPtr& context, const " +
          GetParamterTypeWithNamespace(method->input_type()->full_name()) + "& request, ::trpc::stream::StreamWriter<" +
          GetParamterTypeWithNamespace(method->output_type()->full_name()) + ">* writer) {";
      out += LineFeed(indent + 1) + R"(return ::trpc::Status(-1, "");)";
      out += LineFeed(indent);
      out += "}";
    } else {
      out += "::trpc::Status " + serviceName + "::" + method->name() +
             "(const ::trpc::ServerContextPtr& context, const ::trpc::stream::StreamReader<" +
             GetParamterTypeWithNamespace(method->input_type()->full_name()) +
             ">& reader, ::trpc::stream::StreamWriter<" +
             GetParamterTypeWithNamespace(method->output_type()->full_name()) + ">* writer) {";
      out += LineFeed(indent + 1) + R"(return ::trpc::Status(-1, "");)";
      out += LineFeed(indent);
      out += "}";
    }
  }

  return out;
}

static std::string GenAsyncServiceMethodImp(const ::google::protobuf::ServiceDescriptor* service,
                                            const std::string& pkg, int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto& serviceName = service->name();

  for (int i = 0; i < service->method_count(); ++i) {
    out += LineFeed(indent);
    out += LineFeed(indent);

    auto method = service->method(i);

    const auto input_type = GetParamterTypeWithNamespace(method->input_type()->full_name());
    const auto output_type = GetParamterTypeWithNamespace(method->output_type()->full_name());

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();

    if (!client_stream && !server_stream) {
      out += fmt::format(
          R"(::trpc::Future<{0}> Async{1}::{2}(const ::trpc::ServerContextPtr& context, const {3}* request) {{)",  // NOLINT
          output_type, serviceName, method->name(), input_type);
      out += LineFeed(++indent);
      out += fmt::format(
          R"(return ::trpc::MakeExceptionFuture<{0}>(::trpc::CommonException("Unimplemented"));)",  // NOLINT
          output_type);
      out += LineFeed(--indent);
      out += "}";
    } else if (client_stream && !server_stream) {
      out += fmt::format(
          R"(::trpc::Future<{0}> Async{1}::{2}(const ::trpc::ServerContextPtr& context, const ::trpc::stream::AsyncReaderPtr<{3}>& reader) {{)",  // NOLINT
          output_type, serviceName, method->name(), input_type);
      out += LineFeed(++indent);
      out += fmt::format(
          R"(return ::trpc::MakeExceptionFuture<{0}>(::trpc::CommonException("Unimplemented"));)",  // NOLINT
          output_type);
      out += LineFeed(--indent);
      out += "}";
    } else if (!client_stream && server_stream) {
      out += fmt::format(
          R"(::trpc::Future<> Async{0}::{1}(const ::trpc::ServerContextPtr& context, {2}&& request, const ::trpc::stream::AsyncWriterPtr<{3}>& writer) {{)",  // NOLINT
          serviceName, method->name(), input_type, output_type);
      out += LineFeed(++indent);
      out += R"(return ::trpc::MakeExceptionFuture<>(::trpc::CommonException("Unimplemented"));)";  // NOLINT
      out += LineFeed(--indent);
      out += "}";
    } else {
      out += fmt::format(
          R"(::trpc::Future<> Async{0}::{1}(const ::trpc::ServerContextPtr& context, const ::trpc::stream::AsyncReaderWriterPtr<{2}, {3}>& rw) {{)",  // NOLINT
          serviceName, method->name(), input_type, output_type);
      out += LineFeed(++indent);
      out += R"(return ::trpc::MakeExceptionFuture<>(::trpc::CommonException("Unimplemented"));)";  // NOLINT
      out += LineFeed(--indent);
      out += "}";
    }
  }

  return out;
}

/*
::trpc::Status GreeterServiceProxy::SayHello(const ::trpc::ClientContextPtr& context,
                                             const HelloRequest& request,
                                             HelloReply* response) {
  context->SetFuncName(Greeter_method_names[0][0].data());
  return UnaryInvoke<HelloRequest, HelloReply>(context, request, response);
}
*/
static std::string GenProxySyncCall(const ::google::protobuf::ServiceDescriptor* service,
                                    bool enable_explicit_link_proto, int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto& serviceName = service->name();
  const auto& methodArrayName = serviceName + "_method_names";

  for (int i = 0; i < service->method_count(); ++i) {
    out += LineFeed(indent);
    out += LineFeed(indent);

    auto method = service->method(i);

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();
    if (!client_stream && !server_stream) {
      out += fmt::format(
          "::trpc::Status {0}ServiceProxy::{1}(const ::trpc::ClientContextPtr& context, const {2}& request, {3}* "
          "response) {{",
          serviceName, method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()));
      out += LineFeed(1);
      out += fmt::format("if (context->GetFuncName().empty()) context->SetFuncName({0}[{1}][0].data());",
                         methodArrayName, std::to_string(i));
      if (enable_explicit_link_proto) {
        out += LineFeed(1);
        out += fmt::format(
            "if (context->GetProtobufMethodDescriptor() == nullptr && {0}::GetServiceDescriptor() != nullptr) "
            "context->SetProtobufMethodDescriptor({1}::GetServiceDescriptor()->method({2}));",
            serviceName, serviceName, std::to_string(method->index()));
      }
      out += LineFeed(1);
      out += fmt::format("return UnaryInvoke<{0}, {1}>(context, request, response);",
                         GetParamterTypeWithNamespace(method->input_type()->full_name()),
                         GetParamterTypeWithNamespace(method->output_type()->full_name()));
      out += LineFeed(indent);
      out += "}";
    } else if (client_stream && !server_stream) {
      out += fmt::format(
          "::trpc::stream::StreamWriter<{0}> {1}ServiceProxy::{2}(const ::trpc::ClientContextPtr& context, {3}* "
          "response) {{",
          GetParamterTypeWithNamespace(method->input_type()->full_name()), serviceName, method->name(),
          GetParamterTypeWithNamespace(method->output_type()->full_name()));
      out += LineFeed(1);
      out += fmt::format("if (context->GetFuncName().empty()) context->SetFuncName({0}[{1}][0].data());",
                         methodArrayName, std::to_string(i));
      if (enable_explicit_link_proto) {
        out += LineFeed(1);
        out += fmt::format(
            "if (context->GetProtobufMethodDescriptor() == nullptr && {0}::GetServiceDescriptor() != nullptr) "
            "context->SetProtobufMethodDescriptor({0}::GetServiceDescriptor()->method({1}));",
            serviceName, std::to_string(method->index()));
      }
      out += LineFeed(1);
      out += fmt::format("return StreamInvoke<{0}, {1}>(context, response);",
                         GetParamterTypeWithNamespace(method->input_type()->full_name()),
                         GetParamterTypeWithNamespace(method->output_type()->full_name()));
      out += LineFeed(indent);
      out += "}";
    } else if (!client_stream && server_stream) {
      out += fmt::format(
          "::trpc::stream::StreamReader<{0}> {1}ServiceProxy::{2}(const ::trpc::ClientContextPtr& context, const {3}& "
          "request) {{",
          GetParamterTypeWithNamespace(method->output_type()->full_name()), serviceName, method->name(),
          GetParamterTypeWithNamespace(method->input_type()->full_name()));
      out += LineFeed(1);
      out += fmt::format("if (context->GetFuncName().empty()) context->SetFuncName({0}[{1}][0].data());",
                         methodArrayName, std::to_string(i));
      if (enable_explicit_link_proto) {
        out += LineFeed(1);
        out += fmt::format(
            "if (context->GetProtobufMethodDescriptor() == nullptr && {0}::GetServiceDescriptor() != nullptr) "
            "context->SetProtobufMethodDescriptor({0}::GetServiceDescriptor()->method(1));",
            serviceName, std::to_string(method->index()));
      }
      out += LineFeed(1);
      out += fmt::format("return StreamInvoke<{0}, {1}>(context, request);",
                         GetParamterTypeWithNamespace(method->input_type()->full_name()),
                         GetParamterTypeWithNamespace(method->output_type()->full_name()));
      out += LineFeed(indent);
      out += "}";
    } else {
      out += fmt::format(
          "::trpc::stream::StreamReaderWriter<{0}, {1}> {2}ServiceProxy::{3}(const ::trpc::ClientContextPtr& context) "
          "{{",
          GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()), serviceName, method->name());
      out += LineFeed(1);
      out += fmt::format("if (context->GetFuncName().empty()) context->SetFuncName({0}[{1}][0].data());",
                         methodArrayName, std::to_string(i));
      if (enable_explicit_link_proto) {
        out += LineFeed(1);
        out += fmt::format(
            "if (context->GetProtobufMethodDescriptor() == nullptr && {0}::GetServiceDescriptor() != nullptr) "
            "context->SetProtobufMethodDescriptor({0}::GetServiceDescriptor()->method({1}));",
            serviceName, std::to_string(method->index()));
      }
      out += LineFeed(1);
      out += fmt::format("return StreamInvoke<{0}, {1}>(context);",
                         GetParamterTypeWithNamespace(method->input_type()->full_name()),
                         GetParamterTypeWithNamespace(method->output_type()->full_name()));
      out += LineFeed(indent);
      out += "}";
    }
  }
  return out;
}

static std::string GenProxyOnewayCall(const ::google::protobuf::ServiceDescriptor* service,
                                      bool enable_explicit_link_proto, int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto& serviceName = service->name();
  const auto& methodArrayName = serviceName + "_method_names";

  for (int i = 0; i < service->method_count(); ++i) {
    auto method = service->method(i);

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();
    if (client_stream || server_stream) {
      continue;
    }

    out += LineFeed(indent);
    out += LineFeed(indent);

    out += fmt::format(
        "::trpc::Status {0}ServiceProxy::{1}(const ::trpc::ClientContextPtr& context, const {2}& request) {{",
        serviceName, method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()));
    out += LineFeed(1);
    out += fmt::format("if (context->GetFuncName().empty()) context->SetFuncName({0}[{1}][0].data());",
                       methodArrayName, std::to_string(i));
    if (enable_explicit_link_proto) {
      out += LineFeed(1);
      out += fmt::format(
          "if (context->GetProtobufMethodDescriptor() == nullptr && {0}::GetServiceDescriptor() != nullptr) "
          "context->SetProtobufMethodDescriptor({0}::GetServiceDescriptor()->method({1}));",
          serviceName, std::to_string(method->index()));
    }
    out += LineFeed(1);
    out += fmt::format("return OnewayInvoke<{0}>(context, request);",
                       GetParamterTypeWithNamespace(method->input_type()->full_name()));
    out += LineFeed(indent);
    out += "}";
  }
  return out;
}

/*
::trpc::Future<HelloReply> GreeterServiceProxy::AsyncSayHello(const ::trpc::ClientContextPtr& context,
const HelloRequest& request) { context->SetFuncName(Greeter_method_names[0][0].data()); return
AsyncUnaryInvoke<HelloRequest, HelloReply>(context, request);
}
*/
static std::string GenProxyAsyncCall(const ::google::protobuf::ServiceDescriptor* service,
                                     bool enable_explicit_link_proto, int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto& serviceName = service->name();
  const auto& methodArrayName = serviceName + "_method_names";

  for (int i = 0; i < service->method_count(); ++i) {
    auto method = service->method(i);

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();
    if (client_stream || server_stream) {
      continue;
    }

    out += LineFeed(indent);
    out += LineFeed(indent);

    out += fmt::format(
        "::trpc::Future<{0}> {1}ServiceProxy::Async{2}(const ::trpc::ClientContextPtr& context, const {3}& request) {{",
        GetParamterTypeWithNamespace(method->output_type()->full_name()), serviceName, method->name(),
        GetParamterTypeWithNamespace(method->input_type()->full_name()));
    out += LineFeed(indent + 1);
    out += fmt::format("if (context->GetFuncName().empty()) context->SetFuncName({0}[{1}][0].data());",
                       methodArrayName, std::to_string(i));
    if (enable_explicit_link_proto) {
      out += LineFeed(1);
      out += fmt::format(
          "if (context->GetProtobufMethodDescriptor() == nullptr && {0}::GetServiceDescriptor() != nullptr) "
          "context->SetProtobufMethodDescriptor({0}::GetServiceDescriptor()->method({1}));",
          serviceName, std::to_string(method->index()));
    }
    out += LineFeed(indent + 1);
    out += fmt::format("return AsyncUnaryInvoke<{0}, {1}>(context, request);",
                       GetParamterTypeWithNamespace(method->input_type()->full_name()),
                       GetParamterTypeWithNamespace(method->output_type()->full_name()));
    out += LineFeed(indent);
    out += "}";
  }
  return out;
}

static std::string GenAsyncProxyAsyncCall(const ::google::protobuf::ServiceDescriptor* service,
                                          bool enable_explicit_link_proto, int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto& serviceName = service->name();
  const auto& methodArrayName = serviceName + "_method_names";

  for (int i = 0; i < service->method_count(); ++i) {
    auto method = service->method(i);

    const auto& input_type = GetParamterTypeWithNamespace(method->input_type()->full_name());
    const auto& output_type = GetParamterTypeWithNamespace(method->output_type()->full_name());

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();

    out += LineFeed(indent);
    out += LineFeed(indent);

    auto common = [&]() {
      out += fmt::format("if (context->GetFuncName().empty()) context->SetFuncName({0}[{1}][0].data());",
                         methodArrayName, i);
      out += LineFeed(indent);
      if (enable_explicit_link_proto) {
        out += fmt::format(
            "if (context->GetProtobufMethodDescriptor() == nullptr && Async{0}::GetServiceDescriptor() != nullptr) "
            "context->SetProtobufMethodDescriptor(Async{1}::GetServiceDescriptor()->method({2}));",
            serviceName, serviceName, method->index());
        out += LineFeed(indent);
      }
    };

    if (!client_stream && !server_stream) {
      out += fmt::format(
          "::trpc::Future<{0}> Async{1}ServiceProxy::{2}(const ::trpc::ClientContextPtr& context, const {3}& request) "
          "{{",
          output_type, serviceName, method->name(), input_type);
      out += LineFeed(++indent);
      common();
      out += fmt::format("return AsyncUnaryInvoke<{0}, {1}>(context, request);", input_type, output_type);
      out += LineFeed(--indent);
      out += "}";
    } else if (client_stream && !server_stream) {
      out += fmt::format(
          "::trpc::Future<std::pair<::trpc::stream::AsyncWriterPtr<{0}>, ::trpc::Future<{1}>>> "
          "Async{2}ServiceProxy::{3}(const ::trpc::ClientContextPtr& context) {{",
          input_type, output_type, serviceName, method->name());
      out += LineFeed(++indent);
      common();
      out += fmt::format("return AsyncStreamInvoke<{0}, {1}>(context);", input_type, output_type);
      out += LineFeed(--indent);
      out += "}";
    } else if (!client_stream && server_stream) {
      out += fmt::format(
          "::trpc::Future<::trpc::stream::AsyncReaderPtr<{0}>> Async{1}ServiceProxy::{2}(const "
          "::trpc::ClientContextPtr& context, {3}&& request) {{",
          output_type, serviceName, method->name(), input_type);
      out += LineFeed(++indent);
      common();
      out += fmt::format("return AsyncStreamInvoke<{0}, {1}>(context, std::move(request));", input_type, output_type);
      out += LineFeed(--indent);
      out += "}";
    } else {
      out += fmt::format(
          "::trpc::Future<::trpc::stream::AsyncReaderWriterPtr<{0}, {1}>> Async{2}ServiceProxy::{3}(const "
          "::trpc::ClientContextPtr& context) {{",
          output_type, input_type, serviceName, method->name());
      out += LineFeed(++indent);
      common();
      out += fmt::format("return CreateAsyncStream<{0}, {1}>(context);", input_type, output_type);
      out += LineFeed(--indent);
      out += "}";
    }
  }
  return out;
}

// namespace {

// const ::google::protobuf::ServiceDescriptor* file_level_service_descriptors[1];
// void InitServiceDescriptorsOnce() {
//   static std::once_flag f;
//   std::call_once(f, [] {
//     auto file = ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
//         "trpc/examples/helloworld/helloworld.proto");
//     if (file == nullptr) {
//       TRPC_LOG_ERROR("Load pb descriptor failed, options feature will be disabled.");
//       for (int i = 0; i < 1; ++i) {
//         file_level_service_descriptors[i] = nullptr;
//       }
//       return;
//     }
//     for (int i = 0; i < file->service_count(); ++i) {
//       file_level_service_descriptors[i] = file->service(i);
//     }
//   });
// }
//
// }  // namespace
static std::string GenServiceDescriptorLoad(const google::protobuf::FileDescriptor* file) {
  int service_num = file->service_count();
  if (service_num <= 0) {
    return "";
  }

  int indent = 0;
  std::string out;
  out.reserve(8 * 1024);

  out += LineFeed(indent);
  out += LineFeed(indent);
  out += "namespace {";
  out += LineFeed(indent);
  out += LineFeed(indent);
  out += fmt::format("const ::google::protobuf::ServiceDescriptor* file_level_service_descriptors[{0}];",
                     std::to_string(service_num));
  out += LineFeed(indent);
  out += "void InitServiceDescriptorsOnce() {";
  ++indent;
  out += LineFeed(indent);
  out += "static std::once_flag f;";
  out += LineFeed(indent);
  out += "std::call_once(f, [] {";
  ++indent;
  out += LineFeed(indent);

  // If the dependent proto file is at remote,
  // need to remove the external/project name prefix
  std::string filename = file->name();
  std::string prefix = "external/";
  if (filename.find(prefix) == 0) {
    filename = filename.substr(prefix.size());
    filename = filename.substr(filename.find("/") + 1);
  }

  out += fmt::format("auto file = ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(\"{0}\");",
                     filename);
  out += LineFeed(indent);
  out += "if (file == nullptr) {";
  ++indent;
  out += LineFeed(indent);
  out += "TRPC_LOG_ERROR(\"Load pb descriptor failed, options feature will be disabled.\");";
  out += LineFeed(indent);
  out += fmt::format("for (int i = 0; i < {0}; ++i) {{", std::to_string(file->service_count()));
  ++indent;
  out += LineFeed(indent);
  out += "file_level_service_descriptors[i] = nullptr;";
  --indent;
  out += LineFeed(indent);
  out += "}";
  out += LineFeed(indent);
  out += "return;";
  --indent;
  out += LineFeed(indent);
  out += "}";
  out += LineFeed(indent);
  out += "for (int i = 0; i < file->service_count(); ++i) {";
  ++indent;
  out += LineFeed(indent);
  out += "file_level_service_descriptors[i] = file->service(i);";
  --indent;
  out += LineFeed(indent);
  out += "}";
  --indent;
  out += LineFeed(indent);
  out += "});";
  --indent;
  out += LineFeed(indent);
  out += "}";

  out += LineFeed(indent);
  out += "const ::google::protobuf::ServiceDescriptor* InnerGetServiceDescriptor(int index) {";
  ++indent;
  out += LineFeed(indent);
  out += "InitServiceDescriptorsOnce();";
  out += LineFeed(indent);
  out += "return file_level_service_descriptors[index];";
  --indent;
  out += LineFeed(indent);
  out += "}";

  out += LineFeed(indent);
  out += LineFeed(indent);
  out += "}  // namespace";

  return out;
}

static std::string GenGetServiceDescriptor(const ::google::protobuf::ServiceDescriptor* service, bool async) {
  int indent = 0;
  std::string out;
  out.reserve(8 * 1024);

  out += LineFeed(indent);
  out += LineFeed(indent);
  out += fmt::format(R"(const ::google::protobuf::ServiceDescriptor* {0}{1}::GetServiceDescriptor() {{)",
                     async ? "Async" : "", service->name());
  out += LineFeed(++indent);
  out += fmt::format(R"(return InnerGetServiceDescriptor({0});)", service->index());
  out += LineFeed(--indent);
  out += "}";

  return out;
}

std::string GenTrpcSrcFile(const google::protobuf::FileDescriptor* file, const std::string& stub_path,
                           bool enable_explicit_link_proto) {
  std::string out;
  out.reserve(8 * 1024);

  auto pkg = file->package();

  out += GenHeader(file, stub_path);
  out += GenNamespaceBegin(pkg);

  if (enable_explicit_link_proto) {
    out += GenServiceDescriptorLoad(file);
  }

  for (int i = 0; i < file->service_count(); ++i) {
    auto service = file->service(i);
    out += GenServiceMethodNameArray(service, pkg);
    out += GenServiceConDestructor(service, pkg);
    if (enable_explicit_link_proto) {
      out += GenGetServiceDescriptor(service, false);
    }
    out += GenServiceMethodImp(service, pkg);

    // AsyncService
    out += GenAsyncServiceConDestructor(service, pkg);
    if (enable_explicit_link_proto) {
      out += GenGetServiceDescriptor(service, true);
    }
    out += GenAsyncServiceMethodImp(service, pkg);

    // ServiceProxy
    out += GenProxySyncCall(service, enable_explicit_link_proto);
    out += GenProxyAsyncCall(service, enable_explicit_link_proto);
    out += GenProxyOnewayCall(service, enable_explicit_link_proto);

    // AsyncServiceProxy
    out += GenAsyncProxyAsyncCall(service, enable_explicit_link_proto);
  }

  out += LineFeed(0);
  out += GenNamespaceEnd(pkg);

  return out;
}
