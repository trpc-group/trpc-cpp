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

#include "trpc/filter/filter_id_counter.h"

#include "gtest/gtest.h"

namespace trpc::testing {

class FilterIDCounterTest : public ::testing::Test {
 protected:
  // ...
};

// Test the GetNextID function
TEST_F(FilterIDCounterTest, GetNextID) {
  uint32_t initial_id = GetNextFilterID();
  uint32_t next_id = GetNextFilterID();
  ASSERT_EQ(initial_id + 1, next_id);
}

// Test the uniqueness of filter IDs
TEST_F(FilterIDCounterTest, UniqueIDs) {
  std::set<uint32_t> id_set;
  for (int i = 0; i < 1000; ++i) {
    uint32_t new_id = GetNextFilterID();
    ASSERT_TRUE(id_set.insert(new_id).second);
  }
}

// Test the filter ID limit
TEST_F(FilterIDCounterTest, FilterIDLimit) {
  // Reset the filter_id_counter to a value close to the limit
  SetFilterIDCounter(std::numeric_limits<uint16_t>::max() - 10);

  for (int i = 0; i < 10; ++i) {
    uint32_t new_id = GetNextFilterID();
    ASSERT_LT(new_id, std::numeric_limits<uint16_t>::max());
  }
  
  // This call should trigger the TRPC_ASSERT
  ASSERT_DEATH(GetNextFilterID(), "filter_id_counter has reached its maximum value: 65535");
}

}  // namespace trpc::testing
