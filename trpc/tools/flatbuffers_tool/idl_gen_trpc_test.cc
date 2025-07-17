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

#include "trpc/tools/flatbuffers_tool/idl_gen_trpc.h"

#include "gtest/gtest.h"

#include "trpc/tools/flatbuffers_tool/fbs_gen_test_fixture.h"

namespace flatbuffers::testing {

TEST_F(GenerateTrpcCppFixture, GenTrpcOk) {
  std::string path = "";
  std::string filename = "trpc/tools/flatbuffers_tool/testing/greeter";
  std::vector<const char*> include_directories;
  include_directories.push_back("./");
  flatbuffers::Parser* parser = ConstructParser();
  ParseFile(*parser, filename, contents, include_directories);
  bool ret = GenerateCppTRPC(*parser, path, filename);
  ASSERT_TRUE(ret);
  ret = GenerateCppTRPC(*parser, path, filename, true);
  ASSERT_TRUE(ret);
  // check file content
  std::string gen_trpc_h = path + filename + ".trpc.fb.h";
  std::string gen_trpc_cc = path + filename + ".trpc.fb.cc";

  std::string h_conetent, cc_content;
  flatbuffers::LoadFile(gen_trpc_h.c_str(), true, &h_conetent);
  flatbuffers::LoadFile(gen_trpc_cc.c_str(), true, &cc_content);

  std::string expect_h_path = "trpc/tools/flatbuffers_tool/testing/greeter.trpc.fb_h.txt";
  std::string expect_cc_path = "trpc/tools/flatbuffers_tool/testing/greeter.trpc.fb_cc.txt";
  std::string expect_h_conetent, expect_cc_content;
  flatbuffers::LoadFile(expect_h_path.c_str(), true, &expect_h_conetent);
  flatbuffers::LoadFile(expect_cc_path.c_str(), true, &expect_cc_content);
  ASSERT_EQ(expect_h_conetent, h_conetent);
  ASSERT_EQ(expect_cc_content, cc_content);
}

TEST_F(GenerateTrpcCppFixture, GenTrpcNoContentOk) {
  std::string path = "";
  std::string filename = "greeter";
  flatbuffers::Parser* parser = ConstructParser();
  bool ret = GenerateCppTRPC(*parser, path, filename);
  ASSERT_TRUE(ret);
}

TEST_F(GenerateTrpcCppFixture, GenTrpcFailed) {
  std::string path = "";
  std::string filename = "bin/greeter";
  flatbuffers::Parser* parser = ConstructParser();
  std::vector<const char*> include_directories;
  include_directories.push_back("./");
  // error path, expect error
  ParseFile(*parser, filename, contents, include_directories);
  bool ret = GenerateCppTRPC(*parser, path, filename);
  ASSERT_FALSE(ret);
}

}  // namespace flatbuffers::testing
