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

#include "trpc/admin/mutex.h"

#include <string.h>

#include <atomic>
#include <iostream>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/admin/base_funcs.h"

namespace trpc::testing {

namespace {
bool stop = false;
pthread_mutex_t lock;
std::atomic<uint64_t> product_num{0};
std::queue<uint64_t> product_list;
}  // namespace

void InitProduceThread(int num, std::vector<std::shared_ptr<std::thread>>* thread_vec) {
  std::cout << "!!!, InitProduceThread entry" << std::endl;
  for (int i = 0; i < num; i++) {
    auto thread = std::make_shared<std::thread>([]() {
      while (!stop) {
        uint64_t write_num = ++product_num;
        admin::TrpcPthreadMutexLockImpl(&lock);
        product_list.push(write_num);
        if (write_num % 100 == 0) {
          usleep(5000);
        }
        admin::TrpcPthreadMutexUnlockImpl(&lock);
      }
      std::cout << "stop produce thread" << std::endl;
    });
    thread_vec->push_back(thread);
  }
}

void InitConsumeThread(int num, std::vector<std::shared_ptr<std::thread>>* thread_vec) {
  std::cout << "!!!, InitConsumeThread entry" << std::endl;
  for (int i = 0; i < num; i++) {
    auto thread = std::make_shared<std::thread>([]() {
      while (!stop) {
        admin::TrpcPthreadMutexLockImpl(&lock);
        if (!product_list.empty()) {
          uint64_t read_num = product_list.front();
          if (read_num % 200 == 0) {
            usleep(6000);
          }
          product_list.pop();
        }
        admin::TrpcPthreadMutexUnlockImpl(&lock);
      }
      std::cout << "stop consume thread" << std::endl;
    });
    thread_vec->push_back(thread);
  }
}

TEST(MutextTest, Get) {
  pthread_mutex_init(&lock, NULL);
  std::vector<std::shared_ptr<std::thread>> threads;

  admin::SetMaxProfNum(1);
  admin::SetBaseProfileDir("/tmp/admin_test/");

  char prof_name[256];
  std::map<std::string, std::string> mfiles;
  int ret = admin::GetFiles(admin::ProfilingType::kContention, &mfiles);
  EXPECT_EQ(mfiles.size(), 0);
  EXPECT_EQ(ret, 0);

  admin::MakeNewProfName(admin::ProfilingType::kContention, prof_name, sizeof(prof_name), &mfiles);
  std::cout << "prof_name: " << prof_name << std::endl;

  bool bret = admin::ContentionProfiler::CheckSysCallInit();
  EXPECT_EQ(bret, false);

  bret = admin::ContentionProfilerStart(prof_name);
  EXPECT_EQ(bret, true);

  InitProduceThread(2, &threads);
  InitConsumeThread(2, &threads);

  sleep(3);
  admin::ContentionProfilerStop();

  stop = true;

  std::string res;
  std::string cmd = "ls /tmp/admin_test/";
  ret = admin::ExecCommandByPopen(cmd.c_str(), &res);
  EXPECT_EQ(ret, 0);
  std::cout << "exec cmd: " << cmd << ", res: " << res << std::endl;

  res.clear();
  cmd = "rm -rf /tmp/admin_test";
  ret = admin::ExecCommandByPopen(cmd.c_str(), &res);
  EXPECT_EQ(ret, 0);
  std::cout << "exec cmd: " << cmd << ", res: " << res << std::endl;

  for (auto thread : threads) {
    if (thread->joinable()) {
      thread->join();
    }
  }
}

}  // namespace trpc::testing
