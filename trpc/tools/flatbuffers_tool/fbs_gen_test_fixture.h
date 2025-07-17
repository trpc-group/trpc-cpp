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

#pragma once

#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

#include "gtest/gtest.h"

namespace flatbuffers::testing {

class GenerateTrpcCppFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    std::string fbs_file = "trpc/tools/flatbuffers_tool/testing/greeter.fbs";
    flatbuffers::LoadFile(fbs_file.c_str(), true, &contents);
  }

  static void TearDownTestCase() {
    while (!parsers.empty()) {
      flatbuffers::Parser* parser = parsers.back();
      parsers.pop_back();
      delete parser;
    }
  }

  static flatbuffers::Parser* ConstructParser() {
    flatbuffers::IDLOptions opts;
    SetParser(opts);
    flatbuffers::Parser* parser = new flatbuffers::Parser(opts);
    parsers.push_back(parser);
    return parser;
  }

  static void SetParser(flatbuffers::IDLOptions& opts) {
    opts.use_flexbuffers = false;
    opts.strict_json = false;
    opts.output_default_scalars_in_json = false;
    opts.indent_step = 2;
    opts.output_enum_identifiers = true;
    opts.prefixed_enums = true;
    opts.scoped_enums = false;
    opts.include_dependence_headers = false;
    opts.mutable_buffer = true;
    opts.one_file = false;
    opts.proto_mode = false;
    opts.proto_oneof_union = false;
    opts.generate_all = false;
    opts.skip_unexpected_fields_in_json = false;
    opts.generate_name_strings = false;
    opts.generate_object_based_api = true;
    opts.gen_compare = true;
    opts.cpp_object_api_pointer_type = "flatbufferptr";
    opts.cpp_object_api_string_type = "";
    opts.cpp_object_api_string_flexible_constructor = false;
    opts.gen_nullable = false;
    opts.java_checkerframework = false;
    opts.gen_generated = false;
    opts.object_prefix = "";
    opts.object_suffix = "T";
    opts.union_value_namespacing = true;
    opts.allow_non_utf8 = false;
    opts.natural_utf8 = false;
    opts.include_prefix = "";
    opts.binary_schema_comments = false;
    opts.binary_schema_builtins = false;
    opts.binary_schema_gen_embed = false;
    opts.go_import = "";
    opts.go_namespace = "";
    opts.protobuf_ascii_alike = false;
    opts.size_prefixed = false;
    opts.root_type = "";
    opts.force_defaults = false;
    opts.java_primitive_has_method = false;
    opts.cs_gen_json_serializer = false;
    // opts.cpp_includes = std::vector of length 0, capacity 0
    opts.cpp_std = "";
    opts.proto_namespace_suffix = "";
    opts.filename_suffix = "_generated";
    opts.filename_extension = "";
    opts.mini_reflect = flatbuffers::IDLOptions::kTypesAndNames;
    opts.lang_to_generate = 8;
    opts.set_empty_strings_to_null = true;
    opts.set_empty_vectors_to_null = true;
  }

  static void ParseFile(flatbuffers::Parser& parser, const std::string& filename, const std::string& contents,
                        std::vector<const char*>& include_directories) {
    auto local_include_directory = flatbuffers::StripFileName(filename);
    include_directories.push_back(local_include_directory.c_str());
    include_directories.push_back(nullptr);
    if (!parser.Parse(contents.c_str(), &include_directories[0], filename.c_str())) {
      return;
    }
    if (!parser.error_.empty()) {
      return;
    }
    include_directories.pop_back();
    include_directories.pop_back();
  }

 protected:
  static std::vector<flatbuffers::Parser*> parsers;
  static std::string contents;
};

std::vector<flatbuffers::Parser*> GenerateTrpcCppFixture::parsers;
std::string GenerateTrpcCppFixture::contents = "";

}  // namespace flatbuffers::testing
