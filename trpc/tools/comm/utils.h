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

#pragma once

#include <string>
#include <vector>

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"

namespace trpc::tools {

extern const char* kDeclaration;
extern const char* kIndent;

std::string ProtoFileBaseName(const std::string& fullName);

std::string ProtoFileAppName(const std::string& packageName);

std::string ProtoFileServerName(const std::string& packageName);

std::string LineFeed(int indent = 0);

std::vector<std::string> SepStr(const std::string& sStr, const std::string& sSep);

std::string GenNamespaceStr(const std::string& ns);

std::string GenNamespaceBegin(const std::string& ns);

std::string GenNamespaceEnd(const std::string& ns);

std::string GetParamterTypeWithNamespace(const std::string& typeFullName);

bool NeedMock(const std::string& parameter);

bool NeedEnableExplicitLinkProto(const std::string& parameter);

bool NeedGenStubForExternalRepo(const std::string& parameter, std::string* stub_path);

void ParseRelativeProtoImportPath(const std::string& parameter, std::string* proto_include_prefix);

void WriteToFile(google::protobuf::compiler::GeneratorContext* context, const std::string& file_name,
                 const std::string& file_context);

bool ContainStreamMethod(const google::protobuf::FileDescriptor* file);

bool CheckProtoFileValid(const google::protobuf::FileDescriptor* file, std::string* error);

}  // namespace trpc::tools
