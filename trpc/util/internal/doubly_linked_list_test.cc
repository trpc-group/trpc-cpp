//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/internal/doubly_linked_list_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/internal/doubly_linked_list.h"

#include "gtest/gtest.h"

namespace trpc::testing {

struct DoubleLinkedData {
  DoublyLinkedListEntry chain;
  int x;
};

TEST(DoublyLinkedList, All) {
  DoublyLinkedList<DoubleLinkedData, &DoubleLinkedData::chain> list;

  auto* c = new DoubleLinkedData;
  c->x = 10;
  list.push_back(c);

  c = new DoubleLinkedData;
  c->x = 11;
  list.push_back(c);

  c = new DoubleLinkedData;
  c->x = 9;
  list.push_front(c);

  c = new DoubleLinkedData;
  c->x = 8;
  list.push_front(c);

  ASSERT_FALSE(list.empty());
  ASSERT_EQ(4, list.size());
  ASSERT_EQ(8, list.front().x);
  ASSERT_EQ(11, list.back().x);

  DoubleLinkedData tmp;
  tmp.x = 7;
  list.push_front(&tmp);

  c = new DoubleLinkedData;
  c->x = 6;
  list.push_front(c);

  ASSERT_TRUE(list.erase(&tmp));
  ASSERT_EQ(6, list.front().x);
  ASSERT_FALSE(list.erase(&tmp));
  ASSERT_EQ(6, list.front().x);

  delete list.pop_front();
  for (int i = 8; i <= 11; ++i) {
    ASSERT_EQ(i, list.front().x);
    delete list.pop_front();
  }
}

TEST(DoublyLinkedList, Splice) {
  DoublyLinkedList<DoubleLinkedData, &DoubleLinkedData::chain> list;

  auto* c = new DoubleLinkedData;
  c->x = 1;
  list.push_back(c);

  DoublyLinkedList<DoubleLinkedData, &DoubleLinkedData::chain> list2;
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(1, list.back().x);

  list.splice(std::move(list2));
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(1, list.back().x);

  c = new DoubleLinkedData;
  c->x = 2;
  list.push_back(c);
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(2, list.back().x);

  list.splice(std::move(list2));
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(2, list.back().x);

  c = new DoubleLinkedData;
  c->x = 3;
  list.push_back(c);
  list.splice(std::move(list2));
  ASSERT_TRUE(list2.empty());
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(3, list.back().x);

  c = new DoubleLinkedData;
  c->x = 4;
  list.push_back(c);
  c = new DoubleLinkedData;
  c->x = 5;
  list.push_back(c);
  list.splice(std::move(list2));
  ASSERT_TRUE(list2.empty());
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(5, list.back().x);

  for (int i = 1; i <= 5; ++i) {
    ASSERT_EQ(i, list.front().x);
    delete list.pop_front();
  }
}

TEST(DoublyLinkedList, Swap) {
  DoublyLinkedList<DoubleLinkedData, &DoubleLinkedData::chain> list;

  auto* c = new DoubleLinkedData;
  c->x = 1;
  list.push_back(c);
  c = new DoubleLinkedData;
  c->x = 2;
  list.push_back(c);
  c = new DoubleLinkedData;
  c->x = 3;
  list.push_back(c);
  c = new DoubleLinkedData;
  c->x = 4;
  list.push_back(c);
  ASSERT_EQ(4, list.size());
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(4, list.back().x);

  DoublyLinkedList<DoubleLinkedData, &DoubleLinkedData::chain> list2;
  list.swap(list2);
  ASSERT_TRUE(list.empty());
  ASSERT_EQ(4, list2.size());
  ASSERT_EQ(1, list2.front().x);
  ASSERT_EQ(4, list2.back().x);

  list.swap(list2);
  ASSERT_TRUE(list2.empty());
  ASSERT_EQ(4, list.size());
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(4, list.back().x);

  c = new DoubleLinkedData;
  c->x = 5;
  list2.push_back(c);
  c = new DoubleLinkedData;
  c->x = 6;
  list2.push_back(c);
  c = new DoubleLinkedData;
  c->x = 7;
  list2.push_back(c);
  c = new DoubleLinkedData;
  c->x = 8;
  list2.push_back(c);

  list.swap(list2);
  ASSERT_EQ(4, list2.size());
  ASSERT_EQ(1, list2.front().x);
  ASSERT_EQ(4, list2.back().x);
  ASSERT_EQ(4, list.size());
  ASSERT_EQ(5, list.front().x);
  ASSERT_EQ(8, list.back().x);

  for (int i = 1; i <= 4; ++i) {
    ASSERT_EQ(i, list2.front().x);
    delete list2.pop_front();
  }
  for (int i = 5; i <= 8; ++i) {
    ASSERT_EQ(i, list.front().x);
    delete list.pop_front();
  }

  ASSERT_TRUE(list.empty());
  ASSERT_TRUE(list2.empty());
}

TEST(DoublyLinkedList, Iterator) {
  DoublyLinkedList<DoubleLinkedData, &DoubleLinkedData::chain> list;

  auto* c = new DoubleLinkedData;
  c->x = 4;
  list.push_back(c);

  c = new DoubleLinkedData;
  c->x = 5;
  list.push_back(c);

  c = new DoubleLinkedData;
  c->x = 6;
  list.push_back(c);

  c = new DoubleLinkedData;
  c->x = 3;
  list.push_front(c);

  c = new DoubleLinkedData;
  c->x = 2;
  list.push_front(c);

  c = new DoubleLinkedData;
  c->x = 1;
  list.push_front(c);

  int i = 1;
  for (auto v : list) {
    ASSERT_EQ(i++, v.x);
  }
  while (!list.empty()) {
    delete list.pop_front();
  }
}

TEST(DoublyLinkedList, ConstIterator) {
  DoublyLinkedList<DoubleLinkedData, &DoubleLinkedData::chain> list;

  auto* c = new DoubleLinkedData;
  c->x = 4;
  list.push_back(c);

  c = new DoubleLinkedData;
  c->x = 5;
  list.push_back(c);

  c = new DoubleLinkedData;
  c->x = 6;
  list.push_back(c);

  c = new DoubleLinkedData;
  c->x = 3;
  list.push_front(c);

  c = new DoubleLinkedData;
  c->x = 2;
  list.push_front(c);

  c = new DoubleLinkedData;
  c->x = 1;
  list.push_front(c);

  int i = 1;
  for (auto v : std::as_const(list)) {
    ASSERT_EQ(i++, v.x);
  }
  while (!list.empty()) {
    delete list.pop_front();
  }
}

}  // namespace trpc::testing
