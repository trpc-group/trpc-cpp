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

#include "trpc/admin/base_funcs.h"

#include <string.h>

#include <unistd.h>
#include <iostream>
#include <map>
#include <memory>
#include <utility>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(BaseFuncsTest, Test) {
  int ret = 0;
  std::string res = "";

  ret = admin::CheckDisplayType("dot");
  ASSERT_EQ(ret, 0);

  ret = admin::CheckDisplayType("flame");
  ASSERT_EQ(ret, 0);

  ret = admin::CheckDisplayType("text");
  ASSERT_EQ(ret, 0);

  ret = admin::CheckDisplayType("unknown");
  ASSERT_EQ(ret, -1);

  res.clear();
  admin::GetProfDir(admin::ProfilingType::kCpu, &res);
  ASSERT_EQ(res, "cpu");

  res.clear();
  admin::GetProfDir(admin::ProfilingType::kHeap, &res);
  ASSERT_EQ(res, "heap");

  res.clear();
  admin::GetProfDir(admin::ProfilingType::kContention, &res);
  ASSERT_EQ(res, "contention");

  res.clear();
  admin::GetProfDir(static_cast<admin::ProfilingType>(3), &res);
  ASSERT_EQ(res, "unknown");

  res.clear();
  ret = admin::GetCurrProgressName(&res);
  ASSERT_EQ(ret, 0);

  std::string curr_path = "";
  admin::GetCurrentDirectory(&curr_path);
  std::cout << "!!!, curr directory: " << curr_path << std::endl;

  char prof_name[256] = {0};
  std::string r_output = "";
  std::string w_output = "testing...";
  std::string test_dir = "/tmp/admin_test/";

  admin::SetMaxProfNum(1);
  admin::SetBaseProfileDir("/tmp/admin_test/");

  ret = admin::WriteSysvarsData(test_dir + "aa/mmm.txt", w_output);
  ASSERT_EQ(ret, -1);

  ret = admin::ReadSysvarsData(test_dir + "aa/mmm.txt", &r_output);
  ASSERT_EQ(ret, -1);

  std::map<std::string, std::string> mfiles;
  ret = admin::GetFiles(admin::ProfilingType::kCpu, &mfiles);
  ASSERT_EQ(mfiles.size(), 0);
  ASSERT_EQ(ret, 0);

  ret = admin::MakeNewProfName(admin::ProfilingType::kCpu, prof_name, sizeof(prof_name), &mfiles);
  ASSERT_EQ(ret, 0);
  std::cout << "!!!, prof_name: " << prof_name << std::endl;

  ret = admin::WriteCacheFile(prof_name, w_output);
  ASSERT_EQ(ret, 0);

  ret = admin::GetProfileFileContent(prof_name, &r_output);
  ASSERT_EQ(ret, 0);
  ASSERT_EQ(r_output, w_output);

  std::string display_type = admin::kDisplayDot;
  std::string cache_path = prof_name;
  std::string expect_res = cache_path + ".cache/dot";
  ret = admin::MakeProfCacheName(display_type, &cache_path);
  ASSERT_EQ(ret, 0);
  ASSERT_EQ(cache_path, expect_res);

  ret = admin::GetFiles(admin::ProfilingType::kCpu, &mfiles);
  ASSERT_EQ(mfiles.size(), 1);
  ASSERT_EQ(ret, 0);

  char prof_name2[256] = {0};
  ret = admin::MakeNewProfName(admin::ProfilingType::kCpu, prof_name2, sizeof(prof_name2), &mfiles);
  ASSERT_EQ(ret, 0);
  ASSERT_EQ(mfiles.size(), 0);
  std::cout << "!!!, second prof_name: " << prof_name2 << std::endl;

  ret = admin::WritePprofPerlFile(w_output);
  ASSERT_EQ(ret, 0);

  ret = admin::WriteFlameGraphPerlFile(w_output);
  ASSERT_EQ(ret, 0);

  cache_path = prof_name2;
  ret = admin::WriteSysvarsData(cache_path, w_output);
  ASSERT_EQ(ret, 0);

  r_output.clear();
  ret = admin::ReadSysvarsData(cache_path, &r_output);
  ASSERT_EQ(ret, 0);
  ASSERT_EQ(r_output, w_output);

  res.clear();
  admin::DisplayTypeToCmd(admin::kDisplayDot, &res);
  ASSERT_EQ(res, " --dot ");

  res.clear();
  admin::DisplayTypeToCmd(admin::kDisplayFlameGraph, &res);
  ASSERT_EQ(res, " --collapsed ");

  res.clear();
  admin::DisplayTypeToCmd(admin::kDisplayText, &res);
  ASSERT_EQ(res, " --text ");

  display_type = admin::kDisplayFlameGraph;
  cache_path = prof_name2;
  expect_res = cache_path + ".cache/flame";
  ret = admin::MakeProfCacheName(display_type, &cache_path);
  ASSERT_EQ(ret, 0);
  ASSERT_EQ(cache_path, expect_res);

  display_type = admin::kDisplayText;
  cache_path = prof_name2;
  expect_res = cache_path + ".cache/text";
  ret = admin::MakeProfCacheName(display_type, &cache_path);
  ASSERT_EQ(ret, 0);
  ASSERT_EQ(cache_path, expect_res);

  display_type = "unknown";
  cache_path = prof_name2;
  ret = admin::MakeProfCacheName(display_type, &cache_path);
  ASSERT_EQ(ret, -1);

  std::string remove_dir = prof_name2;
  remove_dir += ".cache";
  ret = admin::RemoveDirectory(remove_dir);
  ASSERT_EQ(ret, 0);

  remove_dir = prof_name2;
  remove_dir += "-tttt";
  ret = admin::RemoveDirectory(remove_dir);
  ASSERT_EQ(ret, -1);

  res.clear();
  std::string cmd = "ls " + test_dir;
  ret = admin::ExecCommandByPopen(cmd.c_str(), &res);
  ASSERT_EQ(ret, 0);
  std::cout << "exec cmd: " << cmd << ", res: " << res << std::endl;

  cmd = "rm -rf " + test_dir;
  ret = admin::ExecCommandByPopen(cmd.c_str(), &res);
  ASSERT_EQ(ret, 0);
  std::cout << "exec cmd: " << cmd << ", res: " << res << std::endl;

  std::string str = "";
  admin::GetProcUptime();
  admin::GetGccVersion(&str);
  std::cout << str << std::endl;

  str = "";
  admin::GetKernelVersion(&str);
  std::cout << str << std::endl;

  str = "";
  admin::GetSystemCoreNums(&str);
  std::cout << str << std::endl;

  str = "";
  admin::GetUserName(&str);
  std::cout << str << std::endl;

  str = "";
  admin::GetCurrentDirectory(&str);
  std::cout << str << std::endl;

  char buff[1024] = {0};
  admin::ReadProcCommandLine(buff, sizeof(buff));

  admin::LoadAverage load;
  ret = admin::ReadLoadUsage(&load);
  ASSERT_EQ(ret, 0);

  rusage stat;
  ret = admin::GetProcRusage(&stat);
  ASSERT_EQ(ret, 0);

  admin::ProcStat pstat;
  ret = admin::ReadProcState(&pstat);
  ASSERT_EQ(ret, 0);

  int fd_count = 0;
  ret = admin::GetProcFdCount(&fd_count);
  std::cout << fd_count << std::endl;
  ASSERT_EQ(ret, 0);

  admin::ProcIO pio;
  ret = admin::ReadProcIo(&pio);
  ASSERT_EQ(ret, 0);

  admin::ProcMemory pmem;
  ret = admin::ReadProcMemory(&pmem);
  ASSERT_EQ(ret, 0);

  ASSERT_NE(0, admin::GetSystemPageSize());

  ASSERT_NE(0, admin::GetSystemClockTick());
}

}  // namespace admin::testing
