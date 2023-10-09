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

#include "trpc/admin/sample.h"

#include <string.h>
#include <unistd.h>

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::testing {

TEST(SampleTest, Test) {
  admin::InitSystemVars();
  sleep(1);
  admin::UpdateSystemVars();

  auto max_op = new admin::MaxOperation<int>();
  admin::PassiveMetrics<int, admin::MaxOperation<int>>* proc_req_max =
      new admin::PassiveMetrics<int, admin::MaxOperation<int>>("proc_req_max", max_op);

  auto min_op = new admin::MinOperation<int>();
  admin::PassiveMetrics<int, admin::MinOperation<int>>* proc_req_min =
      new admin::PassiveMetrics<int, admin::MinOperation<int>>("proc_req_min", min_op);

  auto add_op = new admin::AddOperation<int>();
  admin::PassiveMetrics<int, admin::AddOperation<int>>* proc_req_add =
      new admin::PassiveMetrics<int, admin::AddOperation<int>>("proc_req_add", add_op);

  for (int i = 0; i < 2592000; i++) {
    proc_req_max->TakeSample(i);
    proc_req_min->TakeSample(i);
    proc_req_add->TakeSample(i);
  }

  rapidjson::Document doc;
  rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
  rapidjson::Value result(rapidjson::kObjectType);

  admin::GetVarSeriesData("proc_req_add", result, alloc);
  EXPECT_EQ(result["data"].Size(), 174);

  delete proc_req_max;
  delete proc_req_min;
  delete proc_req_add;
}

}  // namespace trpc::testing
