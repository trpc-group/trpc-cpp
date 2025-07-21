//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
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

#include "trpc/tools/comm/utils.h"

using namespace trpc::tools;

static std::string GenHeader(const google::protobuf::FileDescriptor* file, const std::string& stub_path,
                             const std::string& proto_include_prefix) {
  int indent = 0;
  std::string content;
  content.reserve(8 * 1024);

  content += kDeclaration;
  content += "#pragma once";
  content += LineFeed(indent);
  content += LineFeed(indent);

  std::string pbFileBaseName = ProtoFileBaseName(file->name());
  if (stub_path.length() != 0) {
    std::size_t p = pbFileBaseName.find("external/");
    if (p == 0) {
      pbFileBaseName = pbFileBaseName.substr(std::string("external/").length());
      p = pbFileBaseName.find("/");
      pbFileBaseName = pbFileBaseName.substr(p + 1);
    }
  }

  if (proto_include_prefix != "ProtoIncludePrefixNotValid") {
    std::size_t p = pbFileBaseName.rfind("/");
    if (p != std::string::npos) {
      pbFileBaseName = pbFileBaseName.substr(p + 1);
    }
    if (!proto_include_prefix.empty()) {
      pbFileBaseName = proto_include_prefix + "/" + pbFileBaseName;
    }
  }

  content += fmt::format(R"(#include "{0}.pb.h")", pbFileBaseName);

  content += LineFeed(indent);
  content += LineFeed(indent);
  content += R"(#include "trpc/client/rpc_service_proxy.h")";
  content += LineFeed(indent);
  content += R"(#include "trpc/server/rpc_service_impl.h")";
  content += LineFeed(indent);

  return content;
}

/*
class Greeter: public ::trpc::TrpcService {
public:
  Greeter();

  static ::google::protobuf::ServiceDescriptor* GetServiceDescriptor();

  virtual ::trpc::Status SayHello(::trpc::ServerContextPtr context,
                                  const ::HelloRequest* request,
                                  ::HelloReply* response);
  virtual ::trpc::Status ClientStreamSayHello(const ::trpc::ServerContextPtr& context,
                                              const
::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,  // NOLINT
                                              ::trpc::test::helloworld::HelloReply* response);
  virtual ::trpc::Status ServerStreamSayHello(const ::trpc::ServerContextPtr& context,
                                              const ::trpc::test::helloworld::HelloRequest& request,
                                              ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>*
writer);  // NOLINT virtual ::trpc::Status BidiStreamSayHello(const ::trpc::ServerContextPtr& context, const
::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,  // NOLINT
                                            ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer);
// NOLINT
};
*/

std::string GenService(const ::google::protobuf::ServiceDescriptor* desc, bool enable_explicit_link_proto,
                       int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto& name = desc->name();
  const auto& pkg = GenNamespaceStr(desc->file()->package());

  out += LineFeed(indent);
  out += LineFeed(indent);
  out += fmt::format("class {0} : public ::trpc::RpcServiceImpl {{", name);
  out += LineFeed(indent);
  out += " public:";
  out += LineFeed(++indent);
  out += name + "();" + LineFeed(0);

  if (enable_explicit_link_proto) {
    out += LineFeed(1);
    out += "static const ::google::protobuf::ServiceDescriptor* GetServiceDescriptor();";
    out += LineFeed(0);
  }

  for (int i = 0; i < desc->method_count(); ++i) {
    auto method = desc->method(i);
    out += LineFeed(indent);

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();
    if (!client_stream && !server_stream) {
      out += fmt::format(
          "virtual ::trpc::Status {0}(::trpc::ServerContextPtr context, const {1}* request, {2}* response);",
          method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()));
    } else if (client_stream && !server_stream) {
      out += fmt::format(
          "virtual ::trpc::Status {0}(const ::trpc::ServerContextPtr& context, const "
          "::trpc::stream::StreamReader<{1}>& reader, {2}* response);",
          method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()));
    } else if (!client_stream && server_stream) {
      out += fmt::format(
          "virtual ::trpc::Status {0}(const ::trpc::ServerContextPtr& context, const {1}& request, "
          "::trpc::stream::StreamWriter<{2}>* writer);",
          method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()));
    } else {
      out += fmt::format(
          "virtual ::trpc::Status {0}(const ::trpc::ServerContextPtr& context, const "
          "::trpc::stream::StreamReader<{1}>& reader, ::trpc::stream::StreamWriter<{2}>* writer);",
          method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()));
    }
  }
  out += LineFeed(--indent);
  out += "};";

  return out;
}

std::string GenAsyncService(const ::google::protobuf::ServiceDescriptor* desc, bool enable_explicit_link_proto,
                            int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto& name = desc->name();
  const auto& pkg = GenNamespaceStr(desc->file()->package());

  out += LineFeed(indent);
  out += LineFeed(indent);

  out += fmt::format(R"(class Async{0} : public ::trpc::AsyncRpcServiceImpl {{)", name);
  out += LineFeed(indent);
  out += " public:";
  out += LineFeed(++indent);

  out += fmt::format("Async{0}();", name);
  out += LineFeed(indent);

  if (enable_explicit_link_proto) {
    out += LineFeed(indent);
    out += R"(static const ::google::protobuf::ServiceDescriptor* GetServiceDescriptor();)";
    out += LineFeed(indent);
  }

  for (int i = 0; i < desc->method_count(); ++i) {
    auto method = desc->method(i);

    auto input_type = GetParamterTypeWithNamespace(method->input_type()->full_name());
    auto output_type = GetParamterTypeWithNamespace(method->output_type()->full_name());

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();

    out += LineFeed(indent);
    if (!client_stream && !server_stream) {
      out += fmt::format(
          R"(virtual ::trpc::Future<{0}> {1}(const ::trpc::ServerContextPtr& context, const {2}* request);)",  // NOLINT
          output_type, method->name(), input_type);
    } else if (client_stream && !server_stream) {
      out += fmt::format(
          R"(virtual ::trpc::Future<{0}> {1}(const ::trpc::ServerContextPtr& context, const ::trpc::stream::AsyncReaderPtr<{2}>& reader);)",  // NOLINT
          output_type, method->name(), input_type);
    } else if (!client_stream && server_stream) {
      out += fmt::format(
          R"(virtual ::trpc::Future<> {0}(const ::trpc::ServerContextPtr& context, {1}&& request, const ::trpc::stream::AsyncWriterPtr<{2}>& writer);)",  // NOLINT
          method->name(), input_type, output_type);
    } else {
      out += fmt::format(
          R"(virtual ::trpc::Future<> {0}(const ::trpc::ServerContextPtr& context, const ::trpc::stream::AsyncReaderWriterPtr<{1}, {2}>& rw);)",  // NOLINT
          method->name(), input_type, output_type);
    }
  }

  out += LineFeed(--indent);
  out += "};";

  return out;
}

/*
class GreeterServiceProxy: public ::trpc::TrpcServiceProxy {
public:
  GreeterServiceProxy() = default;
  ~GreeterServiceProxy() override = default;

  virtual ::trpc::Status SayHello(::trpc::ClientContextPtr& context,
                                  const HelloRequest& request,
                                  HelloReply* response);
  virtual ::trpc::Future<HelloReply> AsyncSayHello(::trpc::ClientContextPtr& context,
                                                   const HelloRequest& request);
};
*/

std::string GenProxy(const ::google::protobuf::ServiceDescriptor* desc, int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto& name = desc->name();
  std::string proxy = name + "ServiceProxy";

  out += LineFeed(indent);
  out += LineFeed(indent);
  out += fmt::format("class {0} : public ::trpc::RpcServiceProxy {{", proxy);
  out += LineFeed(indent);
  out += " public:";
  ++indent;

  for (int i = 0; i < desc->method_count(); ++i) {
    auto method = desc->method(i);
    out += LineFeed(indent);

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();
    if (!client_stream && !server_stream) {
      out += fmt::format(
          "virtual ::trpc::Status {0}(const ::trpc::ClientContextPtr& context, const {1}& request, {2}* response);",
          method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()));
      out += LineFeed(indent);
      out += fmt::format(
          "virtual ::trpc::Future<{0}> Async{1}(const ::trpc::ClientContextPtr& context, const {2}& request);",
          GetParamterTypeWithNamespace(method->output_type()->full_name()), method->name(),
          GetParamterTypeWithNamespace(method->input_type()->full_name()));
      out += LineFeed(indent);
      out += "// oneway, only send";
      out += LineFeed(indent);
      out += fmt::format("virtual ::trpc::Status {0}(const ::trpc::ClientContextPtr& context, const {1}& request);",
                         method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()));
    } else if (client_stream && !server_stream) {
      out += fmt::format(
          "virtual ::trpc::stream::StreamWriter<{0}> {1}(const ::trpc::ClientContextPtr& context, {2}* response);",
          GetParamterTypeWithNamespace(method->input_type()->full_name()), method->name(),
          GetParamterTypeWithNamespace(method->output_type()->full_name()));
    } else if (!client_stream && server_stream) {
      out += fmt::format(
          "virtual ::trpc::stream::StreamReader<{0}> {1}(const ::trpc::ClientContextPtr& context, const {2}& request);",
          GetParamterTypeWithNamespace(method->output_type()->full_name()), method->name(),
          GetParamterTypeWithNamespace(method->input_type()->full_name()));
    } else {
      out += fmt::format(
          "virtual ::trpc::stream::StreamReaderWriter<{0}, {1}> {2}(const ::trpc::ClientContextPtr& context);",
          GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()), method->name());
    }
  }
  out += LineFeed(--indent);
  out += "};";
  return out;
}

std::string GenAsyncProxy(const ::google::protobuf::ServiceDescriptor* desc, int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  out += LineFeed(indent);
  out += LineFeed(indent);
  out += fmt::format(R"(class Async{0}ServiceProxy : public ::trpc::RpcServiceProxy {{)", desc->name());
  out += LineFeed(indent);
  out += " public:";
  ++indent;

  for (int i = 0; i < desc->method_count(); ++i) {
    auto method = desc->method(i);

    auto input_type = GetParamterTypeWithNamespace(method->input_type()->full_name());
    auto output_type = GetParamterTypeWithNamespace(method->output_type()->full_name());

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();

    out += LineFeed(indent);
    if (!client_stream && !server_stream) {
      out += fmt::format(
          R"(::trpc::Future<{0}> {1}(const ::trpc::ClientContextPtr& context, const {2}& request);)",  // NOLINT
          output_type, method->name(), input_type);
      out += LineFeed(indent);
      out += "// TODO: one-way";
    } else if (client_stream && !server_stream) {
      out += fmt::format(
          R"(::trpc::Future<std::pair<::trpc::stream::AsyncWriterPtr<{0}>, ::trpc::Future<{1}>>> {2}(const ::trpc::ClientContextPtr& context);)",  // NOLINT
          input_type, output_type, method->name());
    } else if (!client_stream && server_stream) {
      out += fmt::format(
          R"(::trpc::Future<::trpc::stream::AsyncReaderPtr<{0}>> {1}(const ::trpc::ClientContextPtr& context, {2}&& request);)",  // NOLINT
          output_type, method->name(), input_type);
    } else {
      out += fmt::format(
          R"(::trpc::Future<::trpc::stream::AsyncReaderWriterPtr<{0}, {1}>> {2}(const ::trpc::ClientContextPtr& context);)",  // NOLINT
          output_type, input_type, method->name());
    }
  }

  out += LineFeed(--indent);
  out += "};";
  return out;
}

std::string GenTrpcHeaderFile(const google::protobuf::FileDescriptor* file, const std::string& stub_path,
                              const std::string& proto_include_prefix, bool enable_explicit_link_proto) {
  std::string out;
  out.reserve(8 * 1024);

  out += GenHeader(file, stub_path, proto_include_prefix);
  out += GenNamespaceBegin(file->package());

  for (int i = 0; i < file->service_count(); ++i) {
    auto service = file->service(i);
    out += GenService(service, enable_explicit_link_proto);
    out += GenAsyncService(service, enable_explicit_link_proto);
    out += GenProxy(service);
    out += GenAsyncProxy(service);
  }

  out += LineFeed(0);
  out += GenNamespaceEnd(file->package());

  return out;
}
