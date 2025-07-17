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

static std::string GenHeader(const std::string& pbFileBaseName, int indent = 0) {
  std::string content;
  content.reserve(8 * 1024);

  content += kDeclaration;
  content += "#pragma once";
  content += LineFeed(indent);
  content += LineFeed(indent);
  content += R"(#include "gmock/gmock.h")";
  content += LineFeed(indent);
  content += LineFeed(indent);
  content += fmt::format(R"(#include "{0}.trpc.pb.h")", pbFileBaseName);
  content += LineFeed(indent);
  return content;
}

// gmock_v1.10.0 new feature for (NEW)MOCK_METHOD

/*
class MockGreeter: public Greeter {
public:
  MOCK_METHOD(::trpc::Status, SayHello, (::trpc::ServerContextPtr, const HelloRequest*, HelloReply*));
};
*/
std::string GenMockService(const ::google::protobuf::ServiceDescriptor* desc, int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto& name = desc->name();
  const auto& namesp = GenNamespaceStr(desc->file()->package());

  out += LineFeed(indent);
  out += LineFeed(indent);
  out += fmt::format("class Mock{0}: public {0} {{", name);
  out += LineFeed(indent);
  out += "public:";

  for (int i = 0; i < desc->method_count(); ++i) {
    auto method = desc->method(i);
    out += LineFeed(indent + 1);

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();
    if (!client_stream && !server_stream) {
      out += fmt::format("MOCK_METHOD(::trpc::Status, {0}, (::trpc::ServerContextPtr, const {1}*, {2}*));",
                         method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()),
                         GetParamterTypeWithNamespace(method->output_type()->full_name()));
    } else if (client_stream && !server_stream) {
      out += fmt::format(
          "MOCK_METHOD(::trpc::Status, {0}, (const ::trpc::ServerContextPtr&, const "
          "::trpc::stream::StreamReader<{1}>&, {2}*));",
          method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()));
    } else if (!client_stream && server_stream) {
      out += fmt::format(
          "MOCK_METHOD(::trpc::Status, {0}, (const ::trpc::ServerContextPtr&, const {1}&, "
          "::trpc::stream::StreamWriter<{2}>*));",
          method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()));
    } else {
      out += fmt::format(
          "MOCK_METHOD(::trpc::Status, {0}, (const ::trpc::ServerContextPtr&, const "
          "::trpc::stream::StreamReader<{1}>&, ::trpc::stream::StreamWriter<{2}>*));",
          method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()));
    }
  }
  out += LineFeed(indent);
  out += "};";

  return out;
}

std::string GenMockAsyncService(const ::google::protobuf::ServiceDescriptor* desc, int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto& name = desc->name();
  const auto& pkg = GenNamespaceStr(desc->file()->package());

  out += LineFeed(indent);
  out += LineFeed(indent);

  out += fmt::format("class MockAsync{0} : public Async{1} {{", name, name);
  out += LineFeed(indent);
  out += " public:";
  out += LineFeed(++indent);

  for (int i = 0; i < desc->method_count(); ++i) {
    auto method = desc->method(i);

    auto input_type = GetParamterTypeWithNamespace(method->input_type()->full_name());
    auto output_type = GetParamterTypeWithNamespace(method->output_type()->full_name());

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();

    out += LineFeed(indent);
    if (!client_stream && !server_stream) {
      out += fmt::format("MOCK_METHOD(::trpc::Future<{0}>, {1}, (const ::trpc::ServerContextPtr&, const {2}*));",
                         output_type, method->name(), input_type);
    } else if (client_stream && !server_stream) {
      out += fmt::format(
          "MOCK_METHOD(::trpc::Future<{0}>, {1}, (const ::trpc::ServerContextPtr&, const "
          "::trpc::stream::AsyncReaderPtr<{2}>&));",
          output_type, method->name(), input_type);
    } else if (!client_stream && server_stream) {
      out += fmt::format(
          "MOCK_METHOD(::trpc::Future<>, {0}, (const ::trpc::ServerContextPtr&, {1}&&, const "
          "::trpc::stream::AsyncWriterPtr<{2}>&));",
          method->name(), input_type, output_type);
    } else {
      out += fmt::format(
          "MOCK_METHOD(::trpc::Future<>, {0}, (const ::trpc::ServerContextPtr&, const "
          "::trpc::stream::AsyncReaderWriterPtr<{1}, {2}>&));",
          method->name(), input_type, output_type);
    }
  }

  out += LineFeed(--indent);
  out += "};";

  return out;
}

/*
class MockGreeterServiceProxy: public GreeterServiceProxy {
public:
  MOCK_METHOD(::trpc::Status, SayHello, (::trpc::ClientContextPtr, const HelloRequest&, HelloReply*));
  MOCK_METHOD(::trpc::Future<HelloReply>, AsyncSayHello, (::trpc::ClientContextPtr, const
HelloRequest&));
};
*/
std::string GenMockProxy(const ::google::protobuf::ServiceDescriptor* desc, int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  const auto& name = desc->name();
  const auto& proxy = name + "ServiceProxy";
  const auto& mockProxy = "Mock" + proxy;

  out += LineFeed(indent);
  out += LineFeed(indent);
  out += fmt::format("class {0}: public {1} {{", mockProxy, proxy);
  out += LineFeed(indent);
  out += "public:";

  for (int i = 0; i < desc->method_count(); ++i) {
    auto method = desc->method(i);
    out += LineFeed(indent + 1);

    bool client_stream = method->client_streaming();
    bool server_stream = method->server_streaming();
    if (!client_stream && !server_stream) {
      out += fmt::format("MOCK_METHOD(::trpc::Status, {0}, (const ::trpc::ClientContextPtr&, const {1}&, {2}*));",
                         method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()),
                         GetParamterTypeWithNamespace(method->output_type()->full_name()));
      out += LineFeed(indent + 1);
      out += LineFeed(indent + 1);
      out += fmt::format("MOCK_METHOD(::trpc::Future<{0}>, Async{1}, (const ::trpc::ClientContextPtr&, const {2}&));",
                         GetParamterTypeWithNamespace(method->output_type()->full_name()), method->name(),
                         GetParamterTypeWithNamespace(method->input_type()->full_name()));
      out += LineFeed(indent + 1);
      out += fmt::format("MOCK_METHOD(::trpc::Status, {0}, (const ::trpc::ClientContextPtr&, const {1}&));",
                         method->name(), GetParamterTypeWithNamespace(method->input_type()->full_name()));
      out += LineFeed(indent + 1);
    } else if (client_stream && !server_stream) {
      out +=
          fmt::format("MOCK_METHOD(::trpc::stream::StreamWriter<{0}>, {1}, (const ::trpc::ClientContextPtr&, {2}*));",
                      GetParamterTypeWithNamespace(method->input_type()->full_name()), method->name(),
                      GetParamterTypeWithNamespace(method->output_type()->full_name()));
    } else if (!client_stream && server_stream) {
      out += fmt::format(
          "MOCK_METHOD(::trpc::stream::StreamReader<{0}>, {1}, (const ::trpc::ClientContextPtr&, const {2}&));",
          GetParamterTypeWithNamespace(method->output_type()->full_name()), method->name(),
          GetParamterTypeWithNamespace(method->input_type()->full_name()));
    } else {
      out += fmt::format(
          "MOCK_METHOD((::trpc::stream::StreamReaderWriter<{0}, {1}>), {2}, (const ::trpc::ClientContextPtr&));",
          GetParamterTypeWithNamespace(method->input_type()->full_name()),
          GetParamterTypeWithNamespace(method->output_type()->full_name()), method->name());
    }
  }
  out += LineFeed(indent);
  out += "};";

  return out;
}

std::string GenMockAsyncProxy(const ::google::protobuf::ServiceDescriptor* desc, int indent = 0) {
  std::string out;
  out.reserve(8 * 1024);

  out += LineFeed(indent);
  out += LineFeed(indent);
  out += fmt::format(R"(class MockAsync{0}ServiceProxy : public Async{1}ServiceProxy {{)", desc->name(), desc->name());
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
      out += fmt::format(R"(MOCK_METHOD(::trpc::Future<{0}>, {1}, (const ::trpc::ClientContextPtr&, const {2}&));)",
                         output_type, method->name(), input_type);
      out += LineFeed(indent);
      out += "// TODO: one-way";
    } else if (client_stream && !server_stream) {
      out += fmt::format(
          R"(MOCK_METHOD(::trpc::Future<std::pair<::trpc::stream::AsyncWriterPtr<{0}>, ::trpc::Future<{1}>>>, {2}, (const ::trpc::ClientContextPtr&));)",
          input_type, output_type, method->name());
    } else if (!client_stream && server_stream) {
      out += fmt::format(
          R"(MOCK_METHOD(::trpc::Future<::trpc::stream::AsyncReaderPtr<{0}>>, {1}, (const ::trpc::ClientContextPtr&, {2}&&));)",
          output_type, method->name(), input_type);
    } else {
      out += fmt::format(
          R"(MOCK_METHOD((::trpc::Future<::trpc::stream::AsyncReaderWriterPtr<{0}, {1}>>), {2}, (const ::trpc::ClientContextPtr&));)",
          output_type, input_type, method->name());
    }
  }

  out += LineFeed(--indent);
  out += "};";
  return out;
}

std::string GenTrpcMockFile(const google::protobuf::FileDescriptor* file, const std::string& stub_path) {
  std::string out;
  out.reserve(8 * 1024);

  std::string pbFileBaseName;
  if (stub_path.length() == 0) {
    pbFileBaseName = ProtoFileBaseName(file->name());
  } else {
    pbFileBaseName = stub_path;
  }
  out += GenHeader(pbFileBaseName);
  out += GenNamespaceBegin(file->package());

  for (int i = 0; i < file->service_count(); ++i) {
    auto service = file->service(i);
    out += GenMockService(service);
    out += GenMockAsyncService(service);
    out += GenMockProxy(service);
    out += GenMockAsyncProxy(service);
  }

  out += LineFeed(0);
  out += GenNamespaceEnd(file->package());

  return out;
}
