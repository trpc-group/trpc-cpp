//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/internal/singly_linked_list_test.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/internal/singly_linked_list.h"

#include "gtest/gtest.h"

namespace trpc::internal::testing {

struct SinglyLinkedData {
  int x;
  SinglyLinkedListEntry chain;
};

TEST(SinglyLinkedList, All) {
  SinglyLinkedList<SinglyLinkedData, &SinglyLinkedData::chain> list;
  list.push_back(new SinglyLinkedData{.x = 10});
  list.push_back(new SinglyLinkedData{.x = 11});
  list.push_front(new SinglyLinkedData{.x = 9});
  list.push_front(new SinglyLinkedData{.x = 8});
  ASSERT_FALSE(list.empty());
  ASSERT_EQ(4, list.size());
  ASSERT_EQ(8, list.front().x);
  ASSERT_EQ(11, list.back().x);

  SinglyLinkedData tmp{.x = 7};
  list.push_front(&tmp);
  list.push_front(new SinglyLinkedData{.x = 6});
  ASSERT_EQ(6, list.front().x);
  delete list.pop_front();
  ASSERT_EQ(7, list.front().x);
  list.pop_front();

  for (int i = 8; i <= 11; ++i) {
    ASSERT_EQ(i, list.front().x);
    delete list.pop_front();
  }
}

TEST(SinglyLinkedList, Splice) {
  SinglyLinkedList<SinglyLinkedData, &SinglyLinkedData::chain> list;
  list.push_back(new SinglyLinkedData{.x = 1});
  SinglyLinkedList<SinglyLinkedData, &SinglyLinkedData::chain> list2;
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(1, list.back().x);
  list.splice(std::move(list2));
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(1, list.back().x);
  list.push_back(new SinglyLinkedData{.x = 2});
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(2, list.back().x);
  list.splice(std::move(list2));
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(2, list.back().x);
  list2.push_back(new SinglyLinkedData{.x = 3});
  list.splice(std::move(list2));
  ASSERT_TRUE(list2.empty());
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(3, list.back().x);
  list2.push_back(new SinglyLinkedData{.x = 4});
  list2.push_back(new SinglyLinkedData{.x = 5});
  list.splice(std::move(list2));
  ASSERT_TRUE(list2.empty());
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(5, list.back().x);

  for (int i = 1; i <= 5; ++i) {
    ASSERT_EQ(i, list.front().x);
    delete list.pop_front();
  }
}

TEST(SinglyLinkedList, Swap) {
  SinglyLinkedList<SinglyLinkedData, &SinglyLinkedData::chain> list;
  list.push_back(new SinglyLinkedData{.x = 1});
  list.push_back(new SinglyLinkedData{.x = 2});
  list.push_back(new SinglyLinkedData{.x = 3});
  list.push_back(new SinglyLinkedData{.x = 4});
  ASSERT_EQ(4, list.size());
  ASSERT_EQ(1, list.front().x);
  ASSERT_EQ(4, list.back().x);

  SinglyLinkedList<SinglyLinkedData, &SinglyLinkedData::chain> list2;
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

  list2.push_back(new SinglyLinkedData{.x = 5});
  list2.push_back(new SinglyLinkedData{.x = 6});
  list2.push_back(new SinglyLinkedData{.x = 7});
  list2.push_back(new SinglyLinkedData{.x = 8});

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

TEST(SinglyLinkedList, Iterator) {
  SinglyLinkedList<SinglyLinkedData, &SinglyLinkedData::chain> list;
  list.push_back(new SinglyLinkedData{.x = 4});
  list.push_back(new SinglyLinkedData{.x = 5});
  list.push_back(new SinglyLinkedData{.x = 6});
  list.push_front(new SinglyLinkedData{.x = 3});
  list.push_front(new SinglyLinkedData{.x = 2});
  list.push_front(new SinglyLinkedData{.x = 1});

  int i = 1;
  for (auto v : list) {
    ASSERT_EQ(i++, v.x);
  }
  while (!list.empty()) {
    delete list.pop_front();
  }
}

TEST(SinglyLinkedList, ConstIterator) {
  SinglyLinkedList<SinglyLinkedData, &SinglyLinkedData::chain> list;
  list.push_back(new SinglyLinkedData{.x = 4});
  list.push_back(new SinglyLinkedData{.x = 5});
  list.push_back(new SinglyLinkedData{.x = 6});
  list.push_front(new SinglyLinkedData{.x = 3});
  list.push_front(new SinglyLinkedData{.x = 2});
  list.push_front(new SinglyLinkedData{.x = 1});

  int i = 1;
  for (auto v : std::as_const(list)) {
    ASSERT_EQ(i++, v.x);
  }
  while (!list.empty()) {
    delete list.pop_front();
  }
}

}  // namespace trpc::internal::testing
