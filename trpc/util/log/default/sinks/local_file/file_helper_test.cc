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

#include "trpc/util/log/default/sinks/local_file/file_helper.h"

#include <string>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"
#include "sys/stat.h"

TEST(file_uitl_test, test_SimplifyDirectory) {
  std::string path = "/data//log";
  std::string result = trpc::SimplifyDirectory(path);
  EXPECT_EQ(result, "/data/log");

  path = "/data/./log/";
  result = trpc::SimplifyDirectory(path);
  EXPECT_EQ(result, "/data/log");

  path = "/../data/log/";
  result = trpc::SimplifyDirectory(path);
  EXPECT_EQ(result, "/data/log");

  path = "/.";
  result = trpc::SimplifyDirectory(path);
  EXPECT_EQ(result, "/");

  path = "/data/.";
  result = trpc::SimplifyDirectory(path);
  EXPECT_EQ(result, "/data");

  path = "/";
  result = trpc::SimplifyDirectory(path);
  EXPECT_EQ(result, "/");

  path = "/data/";
  result = trpc::SimplifyDirectory(path);
  EXPECT_EQ(result, "/data");

  path = "/..";
  result = trpc::SimplifyDirectory(path);
  EXPECT_EQ(result, "/");
}

TEST(file_uitl_test, test_ExtractFileName) {
  std::string full_file_name{""};
  std::string result = trpc::ExtractFileName(full_file_name);
  EXPECT_EQ(result, "");

  full_file_name = "./file_helper.cc";
  result = trpc::ExtractFileName(full_file_name);
  EXPECT_EQ(result, "file_helper.cc");

  full_file_name = "./file_helper.cc";
  result = trpc::ExtractFileName(full_file_name);
  EXPECT_EQ(result, "file_helper.cc");

  full_file_name = "file_helper.cc";
  result = trpc::ExtractFileName(full_file_name);
  EXPECT_EQ(result, "file_helper.cc");
}

TEST(file_uitl_test, test_ExtractFilePath) {
  std::string full_file_name{""};
  std::string result = trpc::ExtractFilePath(full_file_name);
  EXPECT_EQ(result, "./");

  full_file_name = "./file_helper.cc";
  result = trpc::ExtractFilePath(full_file_name);
  EXPECT_EQ(result, "./");

  full_file_name = "/data/log/file_helper.cc";
  result = trpc::ExtractFilePath(full_file_name);
  EXPECT_EQ(result, "/data/log/");
}

TEST(file_uitl_test, test_GetExePath) {
  std::string::size_type pos = 0;
  std::string result = trpc::GetExePath();
  std::cout << "exe_path:" << result << std::endl;
  pos = result.find("trpc/util/log/default/sinks/local_file/file_helper_test");
  bool find_flag = true;
  if (pos == std::string::npos) {
    find_flag = false;
  }
  EXPECT_TRUE(find_flag);
}

TEST(file_uitl_test, test_SplitByExtension) {
  std::string full_file_name{"mylog"};
  std::string basename{""}, ext{""};
  std::tie(basename, ext) = trpc::SplitByExtension(full_file_name);
  EXPECT_EQ(basename, "mylog");
  EXPECT_EQ(ext, "");

  full_file_name = "/data/log/mylog";
  std::tie(basename, ext) = trpc::SplitByExtension(full_file_name);
  EXPECT_EQ(basename, "/data/log/mylog");
  EXPECT_EQ(ext, "");

  full_file_name = "/data/log/mylog/";
  std::tie(basename, ext) = trpc::SplitByExtension(full_file_name);
  EXPECT_EQ(basename, "/data/log/mylog/");
  EXPECT_EQ(ext, "");

  full_file_name = "mylog.";
  std::tie(basename, ext) = trpc::SplitByExtension(full_file_name);
  EXPECT_EQ(basename, "mylog.");
  EXPECT_EQ(ext, "");

  full_file_name = "/data/log/mylog.log";
  std::tie(basename, ext) = trpc::SplitByExtension(full_file_name);
  EXPECT_EQ(basename, "/data/log/mylog");
  EXPECT_EQ(ext, ".log");

  full_file_name = ".mylog";
  std::tie(basename, ext) = trpc::SplitByExtension(full_file_name);
  EXPECT_EQ(basename, ".mylog");
  EXPECT_EQ(ext, "");

  full_file_name = "data/log/.mylog";
  std::tie(basename, ext) = trpc::SplitByExtension(full_file_name);
  EXPECT_EQ(basename, "data/log/.mylog");
  EXPECT_EQ(ext, "");

  full_file_name = "data/log/.mylog.log";
  std::tie(basename, ext) = trpc::SplitByExtension(full_file_name);
  EXPECT_EQ(basename, "data/log/.mylog");
  EXPECT_EQ(ext, ".log");
}

TEST(file_uitl_test, test_ListDirectory) {
  std::vector<std::string> files;
  std::string result = trpc::GetExePath();
  // Return the file name with the full path.
  trpc::ListDirectory(trpc::ExtractFilePath(result), files, false);
  bool flag = false;
  for (size_t i = 0; i < files.size(); i++) {
    if (trpc::ExtractFileName(files[i]) == "file_helper_test") {
      flag = true;
    }
  }
  EXPECT_TRUE(flag);
  EXPECT_GT(files.size(), 1);
}

TEST(file_uitl_test, test_ScanDir) {
  std::vector<std::string> files;
  std::string result = trpc::GetExePath();
  // Return the file name without the path.
  trpc::ScanDir(trpc::ExtractFilePath(result), files, 0, 0);
  bool flag = false;
  for (size_t i = 0; i < files.size(); i++) {
    if (files[i] == "file_helper_test") {
      flag = true;
    }
  }
  EXPECT_TRUE(flag);
  EXPECT_GT(files.size(), 1);
}

TEST(file_uitl_test, test_IsFileExist) {
  std::string exe_path = trpc::GetExePath();
  // Returns a filename without a path
  std::cout << "exe_path：" << exe_path << std::endl;
  bool flag = trpc::IsFileExist(exe_path, S_IFREG);
  EXPECT_TRUE(flag);
  // test for a nonexistent file
  exe_path = "/error/error/error.log";
  flag = trpc::IsFileExist(exe_path, S_IFREG);
  EXPECT_FALSE(flag);
}
