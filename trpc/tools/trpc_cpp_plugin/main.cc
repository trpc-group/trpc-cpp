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

// This is a Proto3 compiler plugin, generates trpc cpp client and service template codes from protobuf IDL file
// See: https://developers.google.com/protocol-buffers/docs/reference/cpp/#google.protobuf.compiler
// for more information on plugins

#include <string>

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/compiler/plugin.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/port_def.inc"

#include "trpc/tools/comm/utils.h"

using namespace trpc::tools;

std::string GenTrpcHeaderFile(const google::protobuf::FileDescriptor* file, const std::string& stub_path,
                              const std::string& proto_include_prefix, bool enable_explicit_link_proto);
std::string GenTrpcSrcFile(const google::protobuf::FileDescriptor* file, const std::string& stub_path,
                           bool enable_explicit_link_proto);
std::string GenTrpcMockFile(const google::protobuf::FileDescriptor* file, const std::string& stub_path);

class TrpcCppGenerator final : public google::protobuf::compiler::CodeGenerator {
 public:
  TrpcCppGenerator() = default;
  ~TrpcCppGenerator() override = default;
  bool Generate(const google::protobuf::FileDescriptor* file, const std::string& parameter,
                google::protobuf::compiler::GeneratorContext* context, std::string* error) const override;

#if PROTOBUF_VERSION >= 3015000
  uint64_t GetSupportedFeatures() const override { return FEATURE_PROTO3_OPTIONAL; }
#endif

 private:
  static bool CheckFile(const google::protobuf::FileDescriptor* file, std::string* error);
};

// Check the inputed proto file
bool TrpcCppGenerator::CheckFile(const google::protobuf::FileDescriptor* file, std::string* error) {
  return CheckProtoFileValid(file, error);
}

// Gen code file
bool TrpcCppGenerator::Generate(const google::protobuf::FileDescriptor* file, const std::string& parameter,
                                google::protobuf::compiler::GeneratorContext* context, std::string* error) const {
  if (!CheckFile(file, error)) {
    return false;
  }

  // Used to generate trpc stub code for external repo which has pb files.
  std::string stub_path;
  bool gen_for_external_repo = NeedGenStubForExternalRepo(parameter, &stub_path);
  // Used to generate special code using options.(such as alias)
  bool enable_explicit_link_proto = NeedEnableExplicitLinkProto(parameter);
  // Used to generate including path for xxx.pb.h files when encountered relative pb import path.
  std::string proto_include_prefix = "ProtoIncludePrefixNotValid";  // placeholder
  ParseRelativeProtoImportPath(parameter, &proto_include_prefix);

  std::string file_name;

  // Generate trpc header file
  if (!gen_for_external_repo) {
    file_name = ProtoFileBaseName(file->name()) + ".trpc.pb.h";
  } else {
    file_name = stub_path + ".trpc.pb.h";
  }
  WriteToFile(context, file_name, GenTrpcHeaderFile(file, stub_path, proto_include_prefix, enable_explicit_link_proto));

  // Generate trpc source file
  if (!gen_for_external_repo) {
    file_name = ProtoFileBaseName(file->name()) + ".trpc.pb.cc";
  } else {
    file_name = stub_path + ".trpc.pb.cc";
  }
  WriteToFile(context, file_name, GenTrpcSrcFile(file, stub_path, enable_explicit_link_proto));

  // Generate trpc test mock file
  if (NeedMock(parameter)) {
    if (!gen_for_external_repo) {
      file_name = ProtoFileBaseName(file->name()) + ".trpc.pb.mock.h";
    } else {
      file_name = stub_path + ".trpc.pb.mock.h";
    }
    auto mock_file_context = GenTrpcMockFile(file, stub_path);
    WriteToFile(context, file_name, mock_file_context);
  }
  return true;
}

int main(int argc, char* argv[]) {
  TrpcCppGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
