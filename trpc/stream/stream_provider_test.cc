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

#include "trpc/stream/stream_provider.h"

#include "gtest/gtest.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

TEST(ErrorStreamProviderTest, ReadAndWriteAndWriteDoneAndStartAndFinish) {
  StreamReaderWriterProviderPtr stream_provider = MakeRefCounted<ErrorStreamProvider>();
  auto msg_buffer = CreateBufferSlow("");
  EXPECT_FALSE(stream_provider->Read(&msg_buffer).OK());
  EXPECT_FALSE(stream_provider->Write(std::move(msg_buffer)).OK());
  EXPECT_FALSE(stream_provider->WriteDone().OK());
  EXPECT_FALSE(stream_provider->Start().OK());
  EXPECT_FALSE(stream_provider->Finish().OK());
}

}  // namespace trpc::testing
