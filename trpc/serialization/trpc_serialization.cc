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

#include "trpc/serialization/trpc_serialization.h"

#include <utility>

#include "trpc/serialization/flatbuffers/fbs_serialization.h"
#include "trpc/serialization/json/json_serialization.h"
#include "trpc/serialization/noop/noop_serialization.h"
#include "trpc/serialization/pb/pb_serialization.h"
#include "trpc/serialization/serialization_factory.h"

namespace trpc::serialization {

bool Init() {
  auto* factory = SerializationFactory::GetInstance();

  // pb
  TRPC_ASSERT(factory->Register(trpc::MakeRefCounted<PbSerialization>()));

  // flatbuffers
  TRPC_ASSERT(factory->Register(trpc::MakeRefCounted<FbsSerialization>()));

  // json
  TRPC_ASSERT(factory->Register(trpc::MakeRefCounted<JsonSerialization>()));

  // noop
  TRPC_ASSERT(factory->Register(trpc::MakeRefCounted<NoopSerialization>()));

  return true;
}

void Destroy() { SerializationFactory::GetInstance()->Clear(); }

}  // namespace trpc::serialization
