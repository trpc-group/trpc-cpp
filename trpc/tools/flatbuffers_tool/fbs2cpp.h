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
// Copy and modify from https://github.com/google/flatbuffers/blob/v23.5.26/grpc/src/compiler/cpp_generator.h.

// fbs2cpp.h/.cc do not directly depend on std/ProtoBuf, such that they
// can be used to generate code for other serialization systems, such as
// FlatBuffers.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "schema_interface.h"

namespace trpc_cpp_generator {

/// @brief Contains all the parameters that are parsed from the command line.
struct Parameters {
  // Puts the service into a namespace
  std::string services_namespace;
  // Use system includes (<>) or local includes ("")
  bool use_system_headers;
  // Prefix to any trpc include
  std::string trpc_search_path;
  // Generate GMOCK code to facilitate unit testing.
  bool generate_mock_code;
  // By default, use "_generated.h"
  std::string message_header_extension{"_generated.h"};
};

/// @brief Return the prologue of the generated header file.
std::string GetHeaderPrologue(File* file, const Parameters& params);

/// @brief Return the includes needed for generated header file.
std::string GetHeaderIncludes(File* file, const Parameters& params);

/// @brief Return the includes needed for generated source file.
std::string GetSourceIncludes(File* file, const Parameters& params);

/// @brief Return the epilogue of the generated header file.
std::string GetHeaderEpilogue(File* file, const Parameters& params);

/// @brief Return the prologue of the generated source file.
std::string GetSourcePrologue(File* file, const Parameters& params);

/// @brief Return the services for generated header file.
std::string GetHeaderServices(File* file, const Parameters& params);

/// @brief Return the services for generated source file.
std::string GetSourceServices(File* file, const Parameters& params);

/// @brief Return the epilogue of the generated source file.
std::string GetSourceEpilogue(File* file, const Parameters& params);

/// @brief Return the prologue of the generated mock file.
std::string GetMockPrologue(File* file, const Parameters& params);

/// @brief Return the includes needed for generated mock file.
std::string GetMockIncludes(File* file, const Parameters& params);

/// @brief Return the services for generated mock file.
std::string GetMockServices(File* file, const Parameters& params);

/// @brief Return the epilogue of generated mock file.
std::string GetMockEpilogue(File* file, const Parameters& params);
}  // namespace trpc_cpp_generator
