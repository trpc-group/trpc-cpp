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

#include "idl_gen_trpc.h"

#include <cstring>
#include <iostream>

#include "parse_fbs_file.h"

namespace flatbuffers {

// The path of the file generated by executing trpc_fbs_library in the flatbuffers.bzl file may have the following
// prefix.
const char* trpc_file_path[] = {"bin/"};

// If the file is generated by Bazel and needs to remove the prefix, the include statements in the fbs file can be
// distinguished.
static std::string GetTrpcFilePath(const std::string& out_path) {
  std::string::size_type position;
  for (uint32_t i = 0; i < sizeof(trpc_file_path) / sizeof(char*); i++) {
    position = out_path.find(trpc_file_path[i]);
    if (position != out_path.npos) {
      return out_path.substr(position + strlen(trpc_file_path[i]));
    }
  }
  return out_path;
}

bool GenerateCppTRPC(const Parser& parser, const std::string& path, const std::string& file_name, bool is_mock) {
  const auto &opts = parser.opts;
  int nservices = 0;
  for (auto it = parser.services_.vec.begin(); it != parser.services_.vec.end(); ++it) {
    if (!(*it)->generated) nservices++;
  }

  bool ret = true;
  if (!nservices) {
    return ret;
  }

  std::string suffix = "";
  suffix += opts.filename_suffix.empty() ? "_generated" : opts.filename_suffix;
  suffix += ".";
  suffix += opts.filename_extension.empty() ? "h" : opts.filename_extension;

  trpc_cpp_generator::Parameters generator_parameters;
  // TODO(wvo): make the other parameters in this struct configurable.
  generator_parameters.use_system_headers = false;
  generator_parameters.message_header_extension = suffix;

  std::string out_path = GetTrpcFilePath(path + file_name);
  FlatBufFile fbfile(parser, out_path, file_name, FlatBufFile::kLanguageCpp);

  std::string header_code = trpc_cpp_generator::GetHeaderPrologue(&fbfile, generator_parameters) +
                            trpc_cpp_generator::GetHeaderIncludes(&fbfile, generator_parameters) +
                            trpc_cpp_generator::GetHeaderServices(&fbfile, generator_parameters) +
                            trpc_cpp_generator::GetHeaderEpilogue(&fbfile, generator_parameters);

  std::string source_code = trpc_cpp_generator::GetSourcePrologue(&fbfile, generator_parameters) +
                            trpc_cpp_generator::GetSourceIncludes(&fbfile, generator_parameters) +
                            trpc_cpp_generator::GetSourceServices(&fbfile, generator_parameters) +
                            trpc_cpp_generator::GetSourceEpilogue(&fbfile, generator_parameters);

  if (!is_mock) {
    return flatbuffers::SaveFile((path + file_name + ".trpc.fb.h").c_str(), header_code, false) &&
           flatbuffers::SaveFile((path + file_name + ".trpc.fb.cc").c_str(), source_code, false);
  }

  if (is_mock) {
    std::string mock_header_codec = trpc_cpp_generator::GetMockPrologue(&fbfile, generator_parameters) +
                                    trpc_cpp_generator::GetMockIncludes(&fbfile, generator_parameters) +
                                    trpc_cpp_generator::GetMockServices(&fbfile, generator_parameters) +
                                    trpc_cpp_generator::GetMockEpilogue(&fbfile, generator_parameters);
    ret = flatbuffers::SaveFile((path + file_name + ".trpc.fb.mock.h").c_str(), mock_header_codec, false);
  }

  return ret && flatbuffers::SaveFile((path + file_name + ".trpc.fb.h").c_str(), header_code, false) &&
         flatbuffers::SaveFile((path + file_name + ".trpc.fb.cc").c_str(), source_code, false);
}

}  // namespace flatbuffers
