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

#include "gtest/gtest.h"

#include "trpc/tools/flatbuffers_tool/fbs2cpp.h"

namespace trpc_cpp_generator::testing {

class MockMethod : public Method {
 public:
  std::string GetLeadingComments(const std::string) const { return "//test"; }
  std::string GetTrailingComments(const std::string) const { return ""; }
  std::vector<std::string> GetAllComments() const { return std::vector<std::string>(); }
  std::string Name() const override { return "MockMethod"; }
  std::string InputTypeName() const override { return "res"; }
  std::string OutputTypeName() const override { return "rsp"; }

  bool GetModuleAndMessagePathInput(std::string* str, std::string generator_file_name, bool generate_in_pb2_std,
                                    std::string import_prefix) const override {
    return true;
  }
  bool GetModuleAndMessagePathOutput(std::string* str, std::string generator_file_name, bool generate_in_pb2_std,
                                     std::string import_prefix) const override {
    return true;
  }

  std::string GetInputTypeName() const override { return "request"; }
  std::string GetOutputTypeName() const override { return "response"; }

  std::vector<std::string> GetInputNamespaceParts() const override { return std::vector<std::string>(); }
  std::vector<std::string> GetOutputNamespaceParts() const override { return std::vector<std::string>(); }

  std::string GetFbBuilder() const override { return "build"; }

  bool NoStreaming() const override { return true; }

  bool ClientStreaming() const override { return false; }

  bool ServerStreaming() const override { return false; }

  bool BidiStreaming() const override { return false; }
};

class MockPrinter : public Printer {
 public:
  MockPrinter(std::string* str) : str_(str) {}
  ~MockPrinter() = default;
  void Print(const std::map<std::string, std::string>& vars, const char* template_string) override {
    std::string s(template_string);
    for (auto it = vars.begin(); it != vars.end(); it++) {
      s += it->first + ":" + it->second;
    }
    *str_ = s;
  }
  void Print(const char* string) override { *str_ = string; }
  void Indent() override {}
  void Outdent() override {}

 private:
  std::string* str_;
};

class MockService : public Service {
 public:
  std::string GetLeadingComments(const std::string) const { return "//test"; }
  std::string GetTrailingComments(const std::string) const { return ""; }
  std::vector<std::string> GetAllComments() const { return std::vector<std::string>(); }
  std::vector<std::string> NamespaceParts() const override {
    std::vector<std::string> np;
    np.push_back("trpc");
    np.push_back("test");
    return np;
  }
  std::string Name() const override { return "MockService"; }
  bool IsInternal() const override { return true; }
  int MethodCount() const override { return 1; }
  std::unique_ptr<const Method> Methods(int i) const { return std::unique_ptr<Method>(new MockMethod()); }
};

class MockFile : public File {
 public:
  std::string GetLeadingComments(const std::string) const { return "//test"; }
  std::string GetTrailingComments(const std::string) const { return ""; }
  std::vector<std::string> GetAllComments() const { return std::vector<std::string>(); }
  std::string FileName() const override { return ""; }
  std::string FileNameWithoutExt() const override { return ""; }
  std::string Package() const override { return "trpc"; }
  std::vector<std::string> PackageParts() const override {
    std::vector<std::string> pp;
    pp.push_back("pp");
    return pp;
  }
  std::string AdditionalHeaders() const override { return "add"; }
  int ServiceCount() const override { return 1; }
  bool IsCurrentFileService(int i) const override { return true; }
  std::unique_ptr<const Service> Services(int i) const { return std::unique_ptr<Service>(new MockService()); }
  std::unique_ptr<Printer> CreatePrinter(std::string* str, const char indentation_type = ' ') const {
    return std::unique_ptr<Printer>(new MockPrinter(str));
  }
};

TEST(GetHeaderPrologue, WithComment) {
  Parameters params;
  File* file = new MockFile();
  std::string res = GetHeaderPrologue(file, params);
  ASSERT_EQ(res, std::string("\nfilename:filename_base:filename_identifier:message_header_ext:_generated.h"));
  delete file;
}

TEST(GetHeaderServices, WithServicesNamespace) {
  Parameters params;
  params.services_namespace = "nstest";
  File* file = new MockFile();
  std::string res = GetHeaderServices(file, params);
  ASSERT_EQ(res, std::string("}  // namespace "
                             "$services_namespace$\n\nMethod:MockMethodPackage:trpc.Request:flatbuffers::trpc::Message<"
                             "res>Response:flatbuffers::trpc::Message<rsp>Service:MockServiceServiceProxy:"
                             "MockServiceServiceProxyns:nstest::prefix:nstestservices_namespace:nstest"));
  delete file;
}

TEST(GetMockPrologue, TestOk) {
  Parameters params;
  params.services_namespace = "nstest";
  File* file = new MockFile();
  std::string res = GetMockPrologue(file, params);
  ASSERT_EQ(
      res,
      std::string(
          "#include \"$filename_base$$service_header_ext$\"\n\nfilename:filename_base:service_header_ext:.trpc.fb.h"));
  delete file;
}

TEST(GetMockIncludes, TestOk) {
  Parameters params;
  params.services_namespace = "nstest";
  File* file = new MockFile();
  std::string res = GetMockIncludes(file, params);
  ASSERT_EQ(res, std::string("\n\npart:pp"));
  delete file;
}

TEST(GetMockServices, TestOk) {
  Parameters params;
  params.services_namespace = "nstest";
  File* file = new MockFile();
  std::string res = GetMockServices(file, params);
  ASSERT_EQ(res,
            std::string("} // namespace "
                        "$services_namespace$\n\nMethod:MockMethodPackage:trpc.Request:flatbuffers::trpc::Message<res>"
                        "Response:flatbuffers::trpc::Message<rsp>Service:MockServicens:nstest::prefix:nstestservices_"
                        "namespace:nstest"));
  delete file;
}

TEST(GetMockEpilogue, TestOk) {
  Parameters params;
  params.services_namespace = "nstest";
  File* file = new MockFile();
  std::string res = GetMockEpilogue(file, params);
  ASSERT_EQ(res, std::string("} // namespace pp\n\n"));
  delete file;
}

}  // namespace trpc_cpp_generator::testing
