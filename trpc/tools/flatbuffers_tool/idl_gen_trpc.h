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

#include "flatbuffers/code_generators.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

#include "fbs2cpp.h"

namespace flatbuffers {

/// @brief Generate trpc stub code by fbs
/// @param parser Parser
/// @param path The path of fbs file
/// @param file_name The name of fbs file
/// @param is_mock Whether to generate mock code
/// @return
bool GenerateCppTRPC(const Parser& parser, const std::string& path, const std::string& file_name, bool is_mock = false);

}  // namespace flatbuffers
