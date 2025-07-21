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

#include "trpc/serialization/flatbuffers/fbs_serialization.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"

#include "trpc/proto/testing/helloworld_generated.h"
#include "trpc/util/flatbuffers/message_fbs.h"
#include "trpc/util/flatbuffers/trpc_fbs.h"

namespace trpc::testing {

using namespace trpc::serialization;

TEST(FbsSerializationTest, FbsSerializationTest) {
  std::unique_ptr<FbsSerialization> fbs_serialization(new FbsSerialization());

  ASSERT_TRUE(fbs_serialization->Type() == kFlatBuffersType);

  flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest> request;
  flatbuffers::trpc::MessageBuilder mb;
  auto name_offset = mb.CreateString("fb req test");
  auto request_offset = trpc::test::helloworld::CreateFbRequest(mb, name_offset);
  mb.Finish(request_offset);
  request = mb.ReleaseMessage<trpc::test::helloworld::FbRequest>();

  void* fbs_request = static_cast<void*>(&request);
  auto buffer = CreateBufferSlow("");

  bool ret = fbs_serialization->Serialize(kFlatBuffers, fbs_request, &buffer);
  ASSERT_TRUE(ret);

  flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest> request_deserialize;
  void* fbs_request_deserialize = static_cast<void*>(&request_deserialize);

  ret = fbs_serialization->Deserialize(&buffer, kFlatBuffers, fbs_request_deserialize);
  ASSERT_TRUE(ret);

  ASSERT_EQ(request.GetRoot()->message()->str(), request_deserialize.GetRoot()->message()->str());
}

}  // namespace trpc::testing
