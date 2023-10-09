//
// Copyright 2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
// Copy and modify from https://github.com/google/flatbuffers/blob/v23.5.26/src/idl_gen_grpc.cpp.

#pragma once

#include "flatbuffers/idl.h"

#include "schema_interface.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4512)  // C4512: 'class' : assignment operator could
// not be generated
#endif

namespace flatbuffers {

/// @brief Parses the mtehod of RPC-Service in fbs file.
class FlatBufMethod : public trpc_cpp_generator::Method {
 public:
  enum Streaming { kNone, kClient, kServer, kBiDi };

  FlatBufMethod(const RPCCall* method);

  ~FlatBufMethod();

  std::string GetLeadingComments(const std::string) const { return ""; }

  std::string GetTrailingComments(const std::string) const { return ""; }

  std::vector<std::string> GetAllComments() const { return method_->doc_comment; }

  std::string Name() const { return method_->name; }

  //  This method need to incorporate namespace for trpc C++ side.
  std::string TRPCType(const StructDef& sd) const { return sd.name; }

  std::vector<std::string> GetInputNamespaceParts() const { return (*method_->request).defined_namespace->components; }

  std::string GetInputTypeName() const { return (*method_->request).name; }

  std::vector<std::string> GetOutputNamespaceParts() const {
    return (*method_->response).defined_namespace->components;
  }

  std::string GetOutputTypeName() const { return (*method_->response).name; }

  /// @brief Gets input parameters.
  /// @param str: input message content
  /// @param gen_file: generator_file_name
  /// @param generate_in_pb2_std: generate_in_pb2_trpc
  /// @param import_prefix: import_prefix
  bool GetModuleAndMessagePathInput(std::string* str, std::string gen_file, bool generate_in_pb2_std,
                                    std::string import_prefix) const {
    return true;
  }

  /// @brief Gets output parameters.
  /// @param message: output message content
  /// @param gen_file: generator_file_name
  /// @param is_pb2_trpc: generate_in_pb2_trpc
  /// @param prefix: import_prefix
  bool GetModuleAndMessagePathOutput(std::string* message, std::string gen_file, bool is_pb2_trpc,
                                     std::string import_prefix) const {
    return true;
  }

  std::string GetFbBuilder() const { return "builder"; }

  std::string InputTypeName() const;

  std::string OutputTypeName() const;

  bool NoStreaming() const { return streaming_ == kNone; }

  bool ClientStreaming() const { return streaming_ == kClient; }

  bool ServerStreaming() const { return streaming_ == kServer; }

  bool BidiStreaming() const { return streaming_ == kBiDi; }

 private:
  const RPCCall* method_;
  Streaming streaming_;
};

/// @brief Parses RPC-Service in fbs file.
class FlatBufService : public trpc_cpp_generator::Service {
 public:
  FlatBufService(const ServiceDef* service);

  ~FlatBufService();

  std::string GetLeadingComments(const std::string) const { return ""; }

  std::string GetTrailingComments(const std::string) const { return ""; }

  std::vector<std::string> GetAllComments() const { return service_->doc_comment; }

  std::vector<std::string> NamespaceParts() const { return service_->defined_namespace->components; }

  std::string Name() const { return service_->name; }

  bool IsInternal() const { return service_->Definition::attributes.Lookup("private") ? true : false; }

  int MethodCount() const { return static_cast<int>(service_->calls.vec.size()); }

  std::unique_ptr<const trpc_cpp_generator::Method> Methods(int i) const;

 private:
  const ServiceDef* service_;
};

/// @brief Prints string.
class FlatBufPrinter : public trpc_cpp_generator::Printer {
 public:
  explicit FlatBufPrinter(std::string* str, const char indentation_type);

  void Print(const std::map<std::string, std::string>& vars, const char* string_template);

  void Print(const char* s);

  void Indent() { indent_++; }

  void Outdent() {
    indent_--;
    FLATBUFFERS_ASSERT(indent_ >= 0);
  }

  void SetIndentationSize(const size_t size) {
    FLATBUFFERS_ASSERT(str_->empty());
    indentation_size_ = size;
  }

 private:
  std::string* str_;
  char escape_char_;
  int indent_;
  size_t indentation_size_;
  char indentation_type_;
};

/// @brief parses flatbuffers file.
class FlatBufFile : public trpc_cpp_generator::File {
 public:
  enum Language { kLanguageGo, kLanguageCpp, kLanguageJava, kLanguagePython, kLanguageSwift };

  FlatBufFile(const Parser& parser, const std::string& path, const std::string& file_name, Language language);

  FlatBufFile& operator=(const FlatBufFile&) = delete;

  std::string GetLeadingComments(const std::string) const { return ""; }

  std::string GetTrailingComments(const std::string) const { return ""; }

  std::vector<std::string> GetAllComments() const { return std::vector<std::string>(); }

  std::string FileName() const { return file_name_; }

  std::string FileNameWithoutExt() const { return StripExtension(file_name_); }

  std::string Package() const { return parser_.current_namespace_->GetFullyQualifiedName(""); }

  std::vector<std::string> PackageParts() const { return parser_.current_namespace_->components; }

  std::string AdditionalHeaders() const;

  std::string MessageHeaderExt() const { return "_generated.h"; }

  std::string ServiceHeaderExt() const { return ".trpc.fb.h"; }

  int ServiceCount() const { return static_cast<int>(parser_.services_.vec.size()); }

  bool IsCurrentFileService(int i) const;

  std::unique_ptr<const trpc_cpp_generator::Service> Services(int i) const {
    return std::unique_ptr<const trpc_cpp_generator::Service>(new FlatBufService(parser_.services_.vec[i]));
  }

  std::unique_ptr<trpc_cpp_generator::Printer> CreatePrinter(std::string* str,
                                                             const char indentation_type = ' ') const {
    return std::unique_ptr<trpc_cpp_generator::Printer>(new FlatBufPrinter(str, indentation_type));
  }

 private:
  const Parser& parser_;
  const std::string& path_;
  const std::string& file_name_;
  const Language language_;
};

}  // namespace flatbuffers
