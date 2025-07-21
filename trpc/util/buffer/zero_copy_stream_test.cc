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

#include "trpc/util/buffer/zero_copy_stream.h"

#include <string>
#include <utility>

#include "gtest/gtest.h"

#include "google/protobuf/util/message_differencer.h"

#include "trpc/util/buffer/testing/message.pb.h"

namespace trpc::testing {

// The following source codes are from flare.
// Copied and modified from
// https://github.com/Tencent/flare/blob/master/flare/base/buffer/zero_copy_stream_test.cc.

TEST(ZeroCopyStream, SerializeAndDeserialize) {
  testing::ComplexMessage src;
  src.set_integer(556);
  src.set_enumeration(testing::ENUM_0);
  src.set_str("short str");
  src.mutable_one()->set_str("a missing string" + std::string(16777216, 'a'));

  NoncontiguousBufferBuilder nbb;
  NoncontiguousBufferOutputStream zbos(&nbb);
  ASSERT_EQ(src.SerializeToZeroCopyStream(&zbos), true);
  zbos.Flush();
  auto serialized = nbb.DestructiveGet();
  NoncontiguousBuffer cp1 = CreateBufferSlow(FlattenSlow(serialized));
  NoncontiguousBuffer cp2 = serialized;

  ASSERT_EQ(src.SerializeAsString().size(), serialized.ByteSize());

  NoncontiguousBuffer splited;
  splited.Append(serialized.Cut(1));
  splited.Append(serialized.Cut(2));
  splited.Append(serialized.Cut(3));
  splited.Append(serialized.Cut(4));
  splited.Append(std::move(serialized));

  for (auto&& e : {&cp1, &cp2, &splited}) {
    NoncontiguousBufferInputStream nbis(e);
    testing::ComplexMessage dest;
    ASSERT_TRUE(dest.ParseFromZeroCopyStream(&nbis));

    ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(src, dest));
  }
}

}  // namespace trpc::testing
