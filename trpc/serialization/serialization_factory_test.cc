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

#include "trpc/serialization/serialization_factory.h"

#include "gtest/gtest.h"

namespace trpc::testing {

using namespace serialization;

class TestSerialization : public serialization::Serialization {
 public:
  TestSerialization() = default;

  ~TestSerialization() override = default;

  SerializationType Type() const override { return 129; }

  bool Serialize(DataType in_type, void* in, NoncontiguousBuffer* out) override {
    return true;
  }

  bool Deserialize(NoncontiguousBuffer* in, DataType out_type, void* out) override {
    return true;
  }
};

TEST(SerializationFactory, SerializationFactoryTest) {
  SerializationPtr p = trpc::MakeRefCounted<TestSerialization>();

  bool ret = SerializationFactory::GetInstance()->Register(p);
  ASSERT_TRUE(ret);

  Serialization* ptr = SerializationFactory::GetInstance()->Get(129);
  ASSERT_TRUE(p.Get() == ptr);

  SerializationFactory::GetInstance()->Clear();

  Serialization* empty = SerializationFactory::GetInstance()->Get(129);
  ASSERT_TRUE(empty == nullptr);
}

}  // namespace trpc::testing
