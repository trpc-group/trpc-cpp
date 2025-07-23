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

#include <time.h>

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "spdlog/details/os.h"
#include "spdlog/fmt/fmt.h"

#include "trpc/util/log/default/sinks/local_file/local_file_sink.h"

namespace trpc::testing {

class MockLocalFileSink : public LocalFileSink {
 public:
  bool WrapRemoveTimeoutDailyLogFile(const std::string& file_name, int reserver_count) {
    return RemoveTimeoutDailyLogFile(file_name, reserver_count);
  }

  bool WrapSplitFileNameTime(const std::string& file_name, const std::string& ext, tm& tm_time) {
    return SplitFileNameTime(file_name, ext, tm_time);
  }
};

int16_t GetTargetFileSize(const std::string& exe_path, const std::string& exe_file_path) {
  std::vector<std::string> files, target_files;
  trpc::ListDirectory(exe_path, files, false);
  std::string basename{""}, ext{""};
  std::tie(basename, ext) = trpc::SplitByExtension(exe_file_path);
  basename.append("_");
  std::cout << "basename:" << basename << "|ext:" << std::endl;
  std::string::size_type pos = 0;
  for (auto& item : files) {
    pos = item.find(basename);
    if (pos == std::string::npos) continue;
    target_files.emplace_back(item);
  }
  return target_files.size();
}

TEST(remove_timeout_daily_file_test, test_RemoveTimeoutDailyLogFile) {
  spdlog::details::file_helper file_helper;
  std::time_t now = time(NULL);
  tm* now_tm = localtime(&now);

  // Create a test file
  int reserver_count = 1;
  int create_count = 2;
  std::string file_name{""};
  MockLocalFileSink local_file_sink;

  // Make sure we don't get negative numbers like (1-2 = -1) on the 1st of every month
  if (create_count <= now_tm->tm_mday) {
    // executable with full path info, e.g. :
    // ....../k8-fastbuild/bin/trpc/log/default/sinks/local_file_sink_test
    std::string exe_file_path = trpc::GetExePath();
    std::cout << "exe_file_path:" << exe_file_path << std::endl;

    for (int i = 0; i < create_count; i++) {
      file_name = fmt::format("{}_{:04d}-{:02d}-{:02d}{}", exe_file_path, now_tm->tm_year + 1900, now_tm->tm_mon + 1,
                              now_tm->tm_mday - i, ".log");
      // Create a log file
      file_helper.open(file_name, false);
    }

    // Verify that the file was created
    // Get executable path, e.g
    // ...../k8-fastbuild/bin/trpc/log/default/sinks
    std::string exe_path = trpc::ExtractFilePath(exe_file_path);
    std::cout << "exe_path:" << exe_path << std::endl;

    int target_file_size = GetTargetFileSize(exe_path, exe_file_path);
    ASSERT_EQ(target_file_size, create_count);

    // Delete stale files
    std::string local_file_name = exe_file_path;
    local_file_name.append(".log");
    bool b_ret = local_file_sink.WrapRemoveTimeoutDailyLogFile(local_file_name, reserver_count);
    ASSERT_TRUE(b_ret);

    // Verify deletion
    target_file_size = GetTargetFileSize(exe_path, exe_file_path);
    ASSERT_EQ(target_file_size, reserver_count);

    std::string not_timeout_filename = fmt::format("{}_{:04d}-{:02d}-{:02d}{}", exe_file_path, now_tm->tm_year + 1900,
                                                   now_tm->tm_mon + 1, now_tm->tm_mday - reserver_count + 1, ".log");

    std::string timeout_filename = fmt::format("{}_{:04d}-{:02d}-{:02d}{}", exe_file_path, now_tm->tm_year + 1900,
                                               now_tm->tm_mon + 1, now_tm->tm_mday - reserver_count, ".log");

    std::cout << "not_timeout_filename:" << not_timeout_filename << ", timeout_filename:" << timeout_filename
              << std::endl;
    ASSERT_TRUE(trpc::IsFileExist(not_timeout_filename, S_IFREG));
    ASSERT_FALSE(trpc::IsFileExist(timeout_filename, S_IFREG));

    // Clean up the created file
    b_ret = local_file_sink.WrapRemoveTimeoutDailyLogFile(local_file_name, 0);
    ASSERT_TRUE(b_ret);
    target_file_size = GetTargetFileSize(exe_path, exe_file_path);
    ASSERT_EQ(target_file_size, 0);
  }
}

TEST(remove_timeout_daily_file_test, test_SplitFileNameTime) {
  MockLocalFileSink local_file_sink;

  // ---------------------test non-standard file name----------------------//
  std::string file_name{"/data/trpc_2021-04-23.log"};
  std::string ext{".log"};
  tm tm_time{0};
  bool b_ret = local_file_sink.WrapSplitFileNameTime(file_name, ext, tm_time);
  ASSERT_TRUE(b_ret);
  ASSERT_EQ(tm_time.tm_year, 2021 - 1900);  // It goes back to 1900
  ASSERT_EQ(tm_time.tm_mon, 4 - 1);         // It starts in 0
  ASSERT_EQ(tm_time.tm_mday, 23);

  // Returns normal without ext
  file_name = "/data/trpc_2021-04-23";
  b_ret = local_file_sink.WrapSplitFileNameTime(file_name, ext, tm_time);
  ASSERT_TRUE(b_ret);
  ASSERT_EQ(tm_time.tm_year, 2021 - 1900);  // It goes back to 1900
  ASSERT_EQ(tm_time.tm_mon, 4 - 1);         // It starts in 0
  ASSERT_EQ(tm_time.tm_mday, 23);

  // ---------------------test non-standard file name----------------------//
  // Without '_'，If trpc-2021-04-23.log returns false
  file_name = "trpc-2021-04-23.log";
  b_ret = local_file_sink.WrapSplitFileNameTime(file_name, ext, tm_time);
  ASSERT_FALSE(b_ret);

  // filenames not in format {:04d}-{:02d}-{:02d}, false
  // Exclude deleting other files like binaries, config files, etc
  file_name = "data/log_test/exe";
  b_ret = local_file_sink.WrapSplitFileNameTime(file_name, ext, tm_time);
  ASSERT_FALSE(b_ret);

  file_name = "data/log_test/exe_2021-04";
  b_ret = local_file_sink.WrapSplitFileNameTime(file_name, ext, tm_time);
  ASSERT_FALSE(b_ret);

  file_name = "data/log_test/exe_2021/04/09";
  b_ret = local_file_sink.WrapSplitFileNameTime(file_name, ext, tm_time);
  ASSERT_FALSE(b_ret);
}

}  // namespace trpc::testing
