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

#include "trpc/util/chrono/time.h"

#include <algorithm>
#include <memory>

#include "gtest/gtest.h"

#include "trpc/util/chrono/chrono.h"

namespace trpc::time {

namespace testing {

class TimeProviderTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(TimeProviderTest, GetSystemNow) {
  uint64_t now_millisecond_1 = GetSystemMilliSeconds();

  uint64_t now_microsecond_1 = GetSystemMicroSeconds();

  uint64_t now_nanosecond_1 = GetSystemNanoSeconds();

  uint64_t now_second_2 = GetNowAsTimeT();

  timeval tv = GetNowAsTimeVal();

  uint64_t now_millisecond_3 = tv.tv_sec * 1000 + tv.tv_usec / 1000;

  uint64_t max_num = std::max(now_nanosecond_1 / 1000000, now_millisecond_1);
  uint64_t min_num = std::min(now_nanosecond_1 / 1000000, now_millisecond_1);

  ASSERT_TRUE((max_num - min_num) <= 1);

  ASSERT_EQ(now_microsecond_1 / 1000, now_millisecond_1);

  ASSERT_EQ(now_millisecond_1 / 1000, now_second_2);

  ASSERT_EQ(now_millisecond_1 / 100, now_millisecond_3 / 100);
  std::string now_micro_str = ConvertMicroSecsToStr(now_microsecond_1);
  std::string now_mill_str = ConvertMillSecsToStr(now_millisecond_1);
  ASSERT_EQ(now_micro_str.substr(0, 20), now_mill_str.substr(0, 20));
}

TEST_F(TimeProviderTest, GetSteadyNow) {
  uint64_t now_millisecond_1 = GetSteadyMilliSeconds();

  uint64_t now_microsecond_1 = GetSteadyMicroSeconds();

  uint64_t now_nanosecond_1 = GetSteadyNanoSeconds();

  uint64_t max_num = std::max(now_nanosecond_1 / 1000000, now_millisecond_1);
  uint64_t min_num = std::min(now_nanosecond_1 / 1000000, now_millisecond_1);

  ASSERT_TRUE((max_num - min_num) <= 1);

  ASSERT_EQ(now_microsecond_1 / 1000, now_millisecond_1);
}

TEST_F(TimeProviderTest, GetDefaultTimestamp) {
  ASSERT_EQ(GetNanoSeconds() / 1000000000, GetSteadyNanoSeconds() / 1000000000);
  ASSERT_EQ(GetMicroSeconds() / 1000000, GetSteadyMicroSeconds() / 1000000);
  ASSERT_EQ(GetMilliSeconds() / 1000, GetSteadyMilliSeconds() / 1000);
}

class TimeStringHelperTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {}
  static void TearDownTestCase() {}
};

TEST_F(TimeStringHelperTest, HttpDateOk) {
  {
    // 0(Thu, 01 Jan 1970 00:00:00 GMT)
    time_t t = 0;
    std::string expect_http_date{"Thu, 01 Jan 1970 00:00:00 GMT"};

    ASSERT_EQ(expect_http_date, ConvertEpochToHttpDate(t));
    ASSERT_EQ(t, ConvertHttpDateToEpoch(expect_http_date));
  }

  {
    // 1625562342(Tue, 06 Jul 2021 09:05:42 GMT)
    time_t t = 1625562342;
    std::string expect_http_date{"Tue, 06 Jul 2021 09:05:42 GMT"};

    ASSERT_EQ(expect_http_date, ConvertEpochToHttpDate(t));
    ASSERT_EQ(t, ConvertHttpDateToEpoch(expect_http_date));
  }

  {
    // bad case(Bad Case Tue, 06 Jul 2021 17:05:42 GMT)
    std::string bad_http_date{"Bad Case Tue, 06 Jul 2021 17:05:42 GMT"};

    ASSERT_EQ(0, ConvertHttpDateToEpoch(bad_http_date));
  }
}

TEST_F(TimeStringHelperTest, TimeGmOk) {
  {
    // 1625562342(Tue, 06 Jul 2021 09:05:42 GMT)
    time_t t = 1625562342;
    std::string http_date{"Tue, 06 Jul 2021 09:05:42 GMT"};
    time_t parsed_t = ConvertHttpDateToEpoch(http_date);
    struct tm* tm = gmtime(&t);

    ASSERT_EQ(t, parsed_t);
    ASSERT_EQ(t, ConvertTmToTimeGm(tm));
    ASSERT_EQ(t, ConvertTmToTimeGmWithoutYDay(tm));
  }
}

}  // namespace testing

}  // namespace trpc::time
