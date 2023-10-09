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

#include "parse_fbs_file.h"

#include "flatbuffers/idl.h"

namespace flatbuffers {

// parse rpc service method in fbs file
FlatBufMethod::FlatBufMethod(const RPCCall* method) : method_(method) {
  streaming_ = kNone;
  auto val = method_->attributes.Lookup("streaming");
  if (val) {
    if (val->constant == "client") streaming_ = kClient;
    if (val->constant == "server") streaming_ = kServer;
    if (val->constant == "bidi") streaming_ = kBiDi;
  }
}

FlatBufMethod::~FlatBufMethod() {}

std::string FlatBufMethod::InputTypeName() const {
  std::string ns = "";
  auto req_parts = (*method_->request).defined_namespace->components;
  for (auto part = req_parts.begin(); part != req_parts.end(); part++) {
    ns = ns + (*part) + "::";
  }
  return ns + TRPCType(*method_->request);
}

std::string FlatBufMethod::OutputTypeName() const {
  std::string ns = "";
  auto parts = (*method_->response).defined_namespace->components;
  for (auto part = parts.begin(); part != parts.end(); part++) {
    ns = ns + (*part) + "::";
  }
  return ns + TRPCType(*method_->response);
}

// paser rpc service in fbs file
FlatBufService::FlatBufService(const ServiceDef* service) : service_(service) {}

FlatBufService::~FlatBufService() {}

std::unique_ptr<const trpc_cpp_generator::Method> FlatBufService::Methods(int i) const {
  return std::unique_ptr<const trpc_cpp_generator::Method>(new FlatBufMethod(service_->calls.vec[i]));
}

// print string
FlatBufPrinter::FlatBufPrinter(std::string* str, const char indentation_type)
    : str_(str), escape_char_('$'), indent_(0), indentation_size_(2), indentation_type_(indentation_type) {}

void FlatBufPrinter::Print(const std::map<std::string, std::string>& vars, const char* string_template) {
  std::string s = string_template;
  // Replace any occurrences of strings in "vars" that are surrounded
  // by the escape character by what they're mapped to.
  size_t pos;
  while ((pos = s.find(escape_char_)) != std::string::npos) {
    // Found an escape char, must also find the closing one.
    size_t pos2 = s.find(escape_char_, pos + 1);
    // If placeholder not closed, ignore.
    if (pos2 == std::string::npos) break;
    auto it = vars.find(s.substr(pos + 1, pos2 - pos - 1));
    // If unknown placeholder, ignore.
    if (it == vars.end()) break;
    // Subtitute placeholder.
    s.replace(pos, pos2 - pos + 1, it->second);
  }
  Print(s.c_str());
}

void FlatBufPrinter::Print(const char* s) {
  if (s == nullptr || *s == '\0') {
    return;
  }
  // Add this string, but for each part separated by \n, add indentation.
  for (;;) {
    // Current indentation.
    str_->insert(str_->end(), indent_ * indentation_size_, indentation_type_);
    // See if this contains more than one line.
    const char* lf = strchr(s, '\n');
    if (lf) {
      (*str_) += std::string(s, lf + 1);
      s = lf + 1;
      if (!*s) break;  // Only continue if there's more lines.
    } else {
      (*str_) += s;
      break;
    }
  }
}

// fbd file to parse
FlatBufFile::FlatBufFile(const Parser& parser, const std::string& path, const std::string& file_name, Language language)
    : parser_(parser), path_(path), file_name_(file_name), language_(language) {}

std::string FlatBufFile::AdditionalHeaders() const {
  if (language_ != kLanguageCpp) {
    return "";
  }
  std::string content;
  content += R"(#include "trpc/common/status.h")";
  content += "\n";
  content += R"(#include "trpc/server/rpc_service_impl.h")";
  content += "\n";
  content += R"(#include "trpc/client/rpc_service_proxy.h")";
  return content;
}

bool FlatBufFile::IsCurrentFileService(int i) const {
  // Get the file where the i-th service is located.
  auto iservice_file = parser_.services_.vec[i]->file;
  // Find the position of a string without an extension.
  size_t index = iservice_file.find_last_of('.');
  // Find the name of the file.
  std::string file_name = (index != std::string::npos) ? iservice_file.substr(0, index) : iservice_file;
  // Compare if the file where the current service is located belongs to the current file.
  if (file_name != path_) {
    return false;
  }
  return true;
}

}  // namespace flatbuffers
