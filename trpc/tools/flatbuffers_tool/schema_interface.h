// 
// Copyright 2015 gRPC authors.
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
// Copy and modify from https://github.com/google/flatbuffers/blob/v23.5.26/grpc/src/compiler/schema_interface.h.


#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace trpc_cpp_generator {

/// @brief A common interface for objects having comments in the source.
/// @return formatted comments to be inserted in generated code.
struct CommentHolder {
  virtual ~CommentHolder() {}
  virtual std::string GetLeadingComments(const std::string prefix) const = 0;
  virtual std::string GetTrailingComments(const std::string prefix) const = 0;
  virtual std::vector<std::string> GetAllComments() const = 0;
};

/// @brief An abstract interface representing a method.
struct Method : public CommentHolder {
  virtual ~Method() {}

  virtual std::string Name() const = 0;

  virtual std::string InputTypeName() const = 0;
  virtual std::string OutputTypeName() const = 0;

  virtual bool GetModuleAndMessagePathInput(std::string* str, std::string generator_file_name, bool generate_in_pb2_std,
                                            std::string import_prefix) const = 0;
  virtual bool GetModuleAndMessagePathOutput(std::string* str, std::string generator_file_name,
                                             bool generate_in_pb2_std, std::string import_prefix) const = 0;

  virtual std::string GetInputTypeName() const = 0;
  virtual std::string GetOutputTypeName() const = 0;

  virtual std::vector<std::string> GetInputNamespaceParts() const = 0;
  virtual std::vector<std::string> GetOutputNamespaceParts() const = 0;

  virtual std::string GetFbBuilder() const = 0;

  virtual bool NoStreaming() const = 0;

  virtual bool ClientStreaming() const = 0;

  virtual bool ServerStreaming() const = 0;

  virtual bool BidiStreaming() const = 0;
};

/// @brief An abstract interface representing a service.
struct Service : public CommentHolder {
  virtual ~Service() {}

  virtual std::vector<std::string> NamespaceParts() const = 0;

  virtual std::string Name() const = 0;

  virtual bool IsInternal() const = 0;

  virtual int MethodCount() const = 0;

  virtual std::unique_ptr<const Method> Methods(int i) const = 0;
};

/// @brief An abstract interface to print string.
class Printer {
 public:
  Printer() {}
  virtual ~Printer() {}

  virtual void Print(const std::map<std::string, std::string>& vars, const char* template_string) = 0;

  virtual void Print(const char* string) = 0;

  virtual void Indent() = 0;

  virtual void Outdent() = 0;
};

/// @brief An interface that allows the source generated to be output using various
///        libraries/idls/serializers.
struct File : public CommentHolder {
  virtual ~File() {}

  virtual std::string FileName() const = 0;

  virtual std::string FileNameWithoutExt() const = 0;

  virtual std::string Package() const = 0;

  virtual std::vector<std::string> PackageParts() const = 0;

  virtual std::string AdditionalHeaders() const = 0;

  virtual int ServiceCount() const = 0;

  virtual bool IsCurrentFileService(int i) const = 0;

  virtual std::unique_ptr<const Service> Services(int i) const = 0;

  virtual std::unique_ptr<Printer> CreatePrinter(std::string* str, const char indentation_type = ' ') const = 0;
};
}  // namespace trpc_cpp_generator
