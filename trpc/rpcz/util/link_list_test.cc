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

#ifdef TRPC_BUILD_INCLUDE_RPCZ

#include "trpc/rpcz/util/link_list.h"

#include "gtest/gtest.h"

#include "trpc/rpcz/span.h"

namespace trpc::testing {

using LinkNodeType = trpc::rpcz::LinkNode<trpc::rpcz::Span>;

/// @brief Test Constructor.
TEST(LinkNodeTest, Construct) {
  LinkNodeType node;
  ASSERT_EQ(node.previous(), node.next());

  LinkNodeType pre_node, next_node;
  LinkNodeType new_node(&pre_node, &next_node);
  ASSERT_EQ(new_node.previous(), &pre_node);
  ASSERT_EQ(new_node.next(), &next_node);
}

/// @brief Test InsertBefore.
TEST(LinkNodeTest, InsertBefore) {
  LinkNodeType node, new_node;
  node.InsertBefore(&new_node);
  ASSERT_EQ(node.next(), &new_node);
}

/// @brief Test InsertBeforeAsList.
TEST(LinkNodeTest, InsertBeforeAsList) {
  LinkNodeType node, new_node;
  node.InsertBeforeAsList(&new_node);
  ASSERT_EQ(node.next(), &new_node);
}

/// @brief Test InsertAfter.
TEST(LinkNodeTest, InsertAfter) {
  LinkNodeType node, new_node;
  node.InsertAfter(&new_node);
  ASSERT_EQ(node.next(), &new_node);
}

/// @brief Test RemoveFromList.
TEST(LinkNodeTest, RemoveFromList) {
  LinkNodeType node, new_node;
  node.InsertBeforeAsList(&new_node);

  new_node.RemoveFromList();
  ASSERT_EQ(node.next(), &node);
}

}  // namespace trpc::testing
#endif
