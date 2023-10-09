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
// Copy and modify from https://github.com/google/flatbuffers/blob/v23.5.26/src/idl_gen_cpp.cpp.
// Copy and modify from https://github.com/google/flatbuffers/blob/v23.5.26/src/flatc_main.cpp.

#include <cstdio>
#include <list>
#include <memory>
#include <string>
#include <string_view>

#include "flatbuffers/code_generator.h"
#include "flatbuffers/flatc.h"
#include "idl_gen_cpp.h"

#include "idl_gen_trpc.h"

static const char* g_program_name = nullptr;

static void Warn(const flatbuffers::FlatCompiler* flatc, const std::string& warn, bool show_exe_name) {
  (void)flatc;
  if (show_exe_name) {
    printf("%s: ", g_program_name);
  }
  fprintf(stderr, "\nwarning:\n  %s\n\n", warn.c_str());
}

static void Error(const flatbuffers::FlatCompiler* flatc, const std::string& err, bool usage, bool show_exe_name) {
  if (show_exe_name) {
    printf("%s: ", g_program_name);
  }
  if (usage && flatc) {
    fprintf(stderr, "%s\n", flatc->GetShortUsageString(g_program_name).c_str());
  }
  fprintf(stderr, "\nerror:\n  %s\n\n", err.c_str());
  exit(1);
}

namespace flatbuffers {

void LogCompilerWarn(const std::string& warn) {
  Warn(static_cast<const flatbuffers::FlatCompiler*>(nullptr), warn, true);
}
void LogCompilerError(const std::string& err) {
  Error(static_cast<const flatbuffers::FlatCompiler*>(nullptr), err, false, true);
}

class TrpcCppCodeGenerator : public CppCodeGenerator {
 public:
  explicit TrpcCppCodeGenerator(bool is_mock) : is_mock_(is_mock) {}

  Status GenerateGrpcCode(const Parser& parser, const std::string& path, const std::string& filename) override {
    if (!GenerateCppTRPC(parser, path, filename, is_mock_)) {
      return Status::ERROR;
    }
    return Status::OK;
  }

 private:
  bool is_mock_;
};

std::unique_ptr<CodeGenerator> NewTrpcCppCodeGenerator(bool is_mock) {
  return std::unique_ptr<TrpcCppCodeGenerator>(new TrpcCppCodeGenerator(is_mock));
}

}  // namespace flatbuffers

int main(int argc, const char* argv[]) {
  const std::string flatbuffers_version(flatbuffers::FLATBUFFERS_VERSION());

  g_program_name = argv[0];

  bool is_mock = false;
  const char** new_argv = new const char*[argc];
  int j = 0;

  for (int i = 0; i < argc; i++) {
    if (std::string_view(argv[i]) == std::string_view("--trpc")) {
      new_argv[j++] = "--grpc";
    } else if (std::string_view(argv[i]) == std::string_view("--is_mock")) {
      is_mock = true;
    } else {
      new_argv[j++] = argv[i];
    }
  }
  argc = j;

  flatbuffers::FlatCompiler::InitParams params;
  params.warn_fn = Warn;
  params.error_fn = Error;

  flatbuffers::FlatCompiler flatc(params);

  flatc.RegisterCodeGenerator(flatbuffers::FlatCOption{"c", "cpp", "", "Generate C++ headers for tables/structs"},
                              flatbuffers::NewTrpcCppCodeGenerator(is_mock));

  // Create the FlatC options by parsing the command line arguments.
  const flatbuffers::FlatCOptions& options = flatc.ParseFromCommandLineArguments(argc, new_argv);

  // Compile with the extracted FlatC options.
  return flatc.Compile(options);
}
