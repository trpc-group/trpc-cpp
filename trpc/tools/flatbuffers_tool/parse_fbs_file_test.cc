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

#include "trpc/tools/flatbuffers_tool/parse_fbs_file.h"

#include "gtest/gtest.h"

#include "trpc/tools/flatbuffers_tool/fbs_gen_test_fixture.h"

namespace flatbuffers::testing {

TEST_F(GenerateTrpcCppFixture, FlatBufFileTest) {
  std::string path = "";
  std::string filename = "trpc/tools/flatbuffers_tool/testing/greeter";
  std::vector<const char*> include_directories;
  include_directories.push_back("./");
  flatbuffers::Parser* parser = ConstructParser();
  ParseFile(*parser, filename, contents, include_directories);
  std::string out_path("trpc/tools/flatbuffers_tool/testing/greeter");
  std::string file_name("trpc/tools/flatbuffers_tool/testing/greeter");
  FlatBufFile fbfile(*parser, out_path, file_name, FlatBufFile::kLanguageCpp);
  // GetLeadingComments
  ASSERT_EQ(fbfile.GetLeadingComments(""), std::string(""));
  ASSERT_EQ(fbfile.GetLeadingComments("test"), std::string(""));
  // GetTrailingComments
  ASSERT_EQ(fbfile.GetLeadingComments(""), std::string(""));
  ASSERT_EQ(fbfile.GetLeadingComments("test"), std::string(""));

  // GetAllComments
  std::vector<std::string> str_vec = fbfile.GetAllComments();
  ASSERT_EQ(str_vec.size(), 0);
  // FileName
  ASSERT_EQ(fbfile.FileName(), file_name);

  // FileNameWithoutExt
  ASSERT_EQ(fbfile.FileNameWithoutExt(), file_name);

  // Package
  ASSERT_EQ(fbfile.Package(), std::string("trpc.test.helloworld"));

  // PackageParts
  str_vec = fbfile.PackageParts();
  ASSERT_EQ(str_vec.size(), 3);
  ASSERT_EQ(str_vec[0], std::string("trpc"));
  ASSERT_EQ(str_vec[1], std::string("test"));
  ASSERT_EQ(str_vec[2], std::string("helloworld"));

  // AdditionalHeaders
  FlatBufFile fbfileadd(*parser, out_path, file_name, FlatBufFile::kLanguageGo);
  std::string content;
  content += R"(#include "trpc/common/status.h")";
  content += "\n";
  content += R"(#include "trpc/server/rpc_service_impl.h")";
  content += "\n";
  content += R"(#include "trpc/client/rpc_service_proxy.h")";
  ASSERT_EQ(fbfile.AdditionalHeaders(), content);
  ASSERT_EQ(fbfileadd.AdditionalHeaders(), std::string(""));

  // ServiceCount
  ASSERT_EQ(fbfile.ServiceCount(), 1);

  // IsCurrentFileService
  ASSERT_TRUE(fbfile.IsCurrentFileService(0));

  FlatBufFile fbfile_curr(*parser, "./trpc/tools/flatbuffers_tool/testing/greeter", file_name, FlatBufFile::kLanguageCpp);
  ASSERT_FALSE(fbfile_curr.IsCurrentFileService(0));
}

TEST_F(GenerateTrpcCppFixture, FlatBufServiceTest) {
  std::string path = "";
  std::string filename = "trpc/tools/flatbuffers_tool/testing/greeter";
  std::vector<const char*> include_directories;
  include_directories.push_back("./");
  flatbuffers::Parser* parser = ConstructParser();
  ParseFile(*parser, filename, contents, include_directories);
  std::string out_path("trpc/tools/flatbuffers_tool/testing/greeter");
  std::string file_name("trpc/tools/flatbuffers_tool/testing/greeter");
  FlatBufFile fbfile(*parser, out_path, file_name, FlatBufFile::kLanguageCpp);
  auto sptr = fbfile.Services(0);
  // GetLeadingComments
  ASSERT_EQ(sptr->GetLeadingComments(""), std::string(""));
  ASSERT_EQ(sptr->GetLeadingComments("test"), std::string(""));
  // GetTrailingComments
  ASSERT_EQ(sptr->GetLeadingComments(""), std::string(""));
  ASSERT_EQ(sptr->GetLeadingComments("test"), std::string(""));

  // GetAllComments
  std::vector<std::string> str_vec = sptr->GetAllComments();
  ASSERT_EQ(str_vec.size(), 0);

  str_vec = sptr->NamespaceParts();
  ASSERT_EQ(str_vec.size(), 3);
  ASSERT_EQ(str_vec[0], std::string("trpc"));
  ASSERT_EQ(str_vec[1], std::string("test"));
  ASSERT_EQ(str_vec[2], std::string("helloworld"));

  ASSERT_EQ(sptr->Name(), std::string("Greeter"));
  ASSERT_EQ(sptr->IsInternal(), 0);
  ASSERT_EQ(sptr->MethodCount(), 2);
}

TEST_F(GenerateTrpcCppFixture, FlatBufMethodTest) {
  std::string path = "";
  std::string filename = "trpc/tools/flatbuffers_tool/testing/greeter";
  std::vector<const char*> include_directories;
  include_directories.push_back("./");
  flatbuffers::Parser* parser = ConstructParser();
  ParseFile(*parser, filename, contents, include_directories);
  std::string out_path("trpc/tools/flatbuffers_tool/testing/greeter");
  std::string file_name("trpc/tools/flatbuffers_tool/testing/greeter");
  FlatBufFile fbfile(*parser, out_path, file_name, FlatBufFile::kLanguageCpp);
  auto sptr = fbfile.Services(0);
  // get method
  auto mptr = sptr->Methods(0);
  const FlatBufMethod* fptr = dynamic_cast<const FlatBufMethod*>(mptr.get());
  // GetLeadingComments
  ASSERT_EQ(fptr->GetLeadingComments(""), std::string(""));
  ASSERT_EQ(fptr->GetLeadingComments("test"), std::string(""));
  // GetTrailingComments
  ASSERT_EQ(fptr->GetLeadingComments(""), std::string(""));
  ASSERT_EQ(fptr->GetLeadingComments("test"), std::string(""));

  // GetAllComments
  std::vector<std::string> str_vec = fptr->GetAllComments();
  ASSERT_EQ(str_vec.size(), 0);

  ASSERT_EQ(fptr->Name(), std::string("SayHello"));
  ASSERT_EQ(fptr->InputTypeName(), std::string("trpc::test::helloworld::HelloRequest"));
  ASSERT_EQ(fptr->OutputTypeName(), std::string("trpc::test::helloworld::HelloReply"));
  // GetInputNamespaceParts
  str_vec = fptr->GetInputNamespaceParts();
  ASSERT_EQ(str_vec.size(), 3);
  ASSERT_EQ(str_vec[0], std::string("trpc"));
  ASSERT_EQ(str_vec[1], std::string("test"));
  ASSERT_EQ(str_vec[2], std::string("helloworld"));

  // GetOutputNamespaceParts
  str_vec = fptr->GetOutputNamespaceParts();
  ASSERT_EQ(str_vec.size(), 3);
  ASSERT_EQ(str_vec[0], std::string("trpc"));
  ASSERT_EQ(str_vec[1], std::string("test"));
  ASSERT_EQ(str_vec[2], std::string("helloworld"));

  // in/out type
  ASSERT_EQ(fptr->GetInputTypeName(), std::string("HelloRequest"));
  ASSERT_EQ(fptr->GetOutputTypeName(), std::string("HelloReply"));
  // GetFbBuilder
  ASSERT_EQ(fptr->GetFbBuilder(), std::string("builder"));
  std::string gen_file("");
  std::string import_prefix("");
  ASSERT_TRUE(fptr->GetModuleAndMessagePathInput(nullptr, gen_file, false, import_prefix));
  ASSERT_TRUE(fptr->GetModuleAndMessagePathOutput(nullptr, gen_file, false, import_prefix));
  // stream
  ASSERT_TRUE(fptr->NoStreaming());
  ASSERT_FALSE(fptr->ClientStreaming());
  ASSERT_FALSE(fptr->ServerStreaming());
  ASSERT_FALSE(fptr->BidiStreaming());
}

TEST_F(GenerateTrpcCppFixture, FlatBufPrinterTest) {
  std::string test("");
  const char indentation_type = ' ';
  FlatBufPrinter fbp = FlatBufPrinter(&test, indentation_type);
  std::map<std::string, std::string> vars;

  const char* string_template_aa = "aa";
  fbp.Print(vars, string_template_aa);
  ASSERT_EQ(test, std::string("aa"));

  const char* string_template = "$";
  fbp.Print(vars, string_template);
  ASSERT_EQ(test, std::string("aa$"));

  const char* string_template_bb = "$bb$";
  fbp.Print(vars, string_template_bb);
  ASSERT_EQ(test, std::string("aa$$bb$"));

  vars.insert({"cc", "aa"});
  const char* string_template_cc = "bb$cc$aa";
  fbp.Print(vars, string_template_cc);
  ASSERT_EQ(test, std::string("aa$$bb$bbaaaa"));
}

}  // namespace flatbuffers
