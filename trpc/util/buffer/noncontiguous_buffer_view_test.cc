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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. flare
// Copyright (C) 2019 Tencent.
// flare is licensed under the BSD 3-Clause License.
//

#include "trpc/util/buffer/noncontiguous_buffer_view.h"

#include "gtest/gtest.h"

#include "trpc/util/algorithm/random.h"

namespace trpc::testing {

// The following source codes are from flare.
// Copied and modified from
// https://github.com/Tencent/flare/blob/master/flare/base/buffer/view_test.cc.

void GetTestBuffer(std::size_t length, std::string& check_str, NoncontiguousBuffer& buffer) {
  // generate the random check_str
  for (std::size_t i = 0; i < length; ++i) {
    check_str += Random<char>(std::numeric_limits<char>::min(), std::numeric_limits<char>::max());
  }

  // genetate the random NoncontiguousBuffer using check_str
  std::size_t pos = 0;
  std::size_t left = length;
  while (left > 0) {
    int buffer_len = Random<int>(left);
    std::string data = check_str.substr(pos, buffer_len);
    pos += buffer_len;
    left -= buffer_len;
    NoncontiguousBufferBuilder builder;
    builder.Append(data);
    buffer.Append(builder.DestructiveGet());
  }
}

TEST(NoncontiguousBufferViewTest, BasicOperator) {
  std::string check_str;
  NoncontiguousBuffer buffer;
  GetTestBuffer(200, check_str, buffer);

  detail::NoncontiguousBufferView wrapper(buffer);
  auto begin = wrapper.begin();
  auto end = wrapper.end();
  std::size_t pos = 0;
  auto iter = wrapper.begin();

  // 1. testing operator*
  ASSERT_EQ(check_str[pos], *iter);

  // 2. testing operator++
  ++iter;
  ++pos;
  ASSERT_EQ(check_str[pos], *iter);

  // 3. testing operator+
  iter = iter + 3;
  pos = pos + 3;
  ASSERT_EQ(check_str[pos], *iter);

  // 4. testing operator!=
  ASSERT_NE(begin, iter);
  ASSERT_NE(end, iter);

  // 5. testing operator==
  auto tmp_iter(iter);
  ASSERT_EQ(tmp_iter, iter);

  // 6. testing operator-
  ASSERT_EQ(pos, iter - begin);
  ASSERT_EQ(buffer.ByteSize(), end - begin);
}

TEST(NoncontiguousBufferViewTest, Traversal) {
  // randomly verify 100 times
  int test_count = 100;
  int test_len = 204800;
  while (--test_count >= 0) {
    std::string check_str;
    NoncontiguousBuffer buffer;
    GetTestBuffer(test_len, check_str, buffer);

    // Traverse the NoncontiguousBuffer and check if each character matches.
    detail::NoncontiguousBufferView wrapper(buffer);
    ASSERT_EQ(wrapper.end() - wrapper.begin(), test_len);
    auto it = wrapper.begin();
    std::size_t count = 0;
    for (; it != wrapper.end(); ++it, ++count) {
      ASSERT_EQ(check_str[count], *it);
      ASSERT_EQ(count, it - wrapper.begin());
    }
    ASSERT_EQ(count, buffer.ByteSize());
    ASSERT_EQ(it, wrapper.end());

    // check operator+
    it = wrapper.begin();
    for (int i = 0; i < test_len; ++i) {
      auto test_it = it + i;
      ASSERT_EQ(*test_it, check_str[i]);
      ASSERT_EQ(test_it - wrapper.begin(), i);
    }
    ASSERT_EQ(it + test_len, wrapper.end());
  }
}

TEST(NoncontiguousBufferViewTest, CrossBorder) {
  NoncontiguousBuffer check_buf;
  detail::NoncontiguousBufferView wrapper(check_buf);
  ASSERT_EQ(wrapper.begin(), wrapper.end());
  auto it = wrapper.begin();
  ++it;
  ++it;
  ASSERT_EQ(it, wrapper.end());
}

}  // namespace trpc::testing
