//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/ref_ptr_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/ref_ptr.h"

#include <atomic>

#include "gtest/gtest.h"

namespace trpc {

struct RefCounted1 {
  std::atomic<int> ref_count{};
  RefCounted1() { ++instances; }
  ~RefCounted1() { --instances; }
  inline static int instances = 0;
};

template <>
struct RefTraits<RefCounted1> {
  static void Reference(RefCounted1* rc) {
    rc->ref_count.fetch_add(1, std::memory_order_relaxed);
  }
  static void Dereference(RefCounted1* rc) {
    if (rc->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      delete rc;
    }
  }
};

struct RefCounted2 : RefCounted<RefCounted2> {
  RefCounted2() { ++instances; }
  ~RefCounted2() { --instances; }
  inline static int instances = 0;
};

struct RefCountedVirtual : RefCounted<RefCountedVirtual> {
  RefCountedVirtual() { ++instances; }
  virtual ~RefCountedVirtual() { --instances; }
  inline static int instances = 0;
};

struct RefCounted3 : RefCountedVirtual {
  RefCounted3() { ++instances; }
  ~RefCounted3() { --instances; }
  inline static int instances = 0;
};

static_assert(!detail::is_ref_counted_directly_v<RefCounted3>);
static_assert(detail::is_ref_counted_indirectly_safe_v<RefCounted3>);

TEST(RefPtr, ReferenceCount) {
  {
    auto ptr = new RefCounted1();
    ptr->ref_count = 0;
    ASSERT_EQ(1, RefCounted1::instances);
    RefPtr p1(ref_ptr, ptr);
    ASSERT_EQ(1, ptr->ref_count);
    {
      RefPtr p2(p1);
      ASSERT_EQ(2, ptr->ref_count);
      RefPtr p3(std::move(p2));
      ASSERT_EQ(2, ptr->ref_count);
    }
    {
      RefPtr p2(p1);
      ASSERT_EQ(2, ptr->ref_count);
      p2.Reset();
      ASSERT_EQ(1, ptr->ref_count);
    }
    {
      RefPtr p2(p1);
      ASSERT_EQ(2, ptr->ref_count);
      auto ptr = p2.Leak();
      ASSERT_EQ(2, ptr->ref_count);
      RefPtr p3(adopt_ptr, ptr);
      ASSERT_EQ(2, ptr->ref_count);
    }
    ASSERT_EQ(1, ptr->ref_count);
  }
  ASSERT_EQ(0, RefCounted1::instances);
}

TEST(RefPtr, RefCounted) {
  {
    auto ptr = new RefCounted2();
    ASSERT_EQ(1, RefCounted2::instances);
    RefPtr p1(adopt_ptr, ptr);
  }
  ASSERT_EQ(0, RefCounted2::instances);
}

TEST(RefPtr, RefCountedVirtualDtor) {
  {
    auto ptr = new RefCounted3();
    ASSERT_EQ(1, RefCounted3::instances);
    ASSERT_EQ(1, RefCountedVirtual::instances);
    RefPtr p1(adopt_ptr, ptr);
  }
  ASSERT_EQ(0, RefCounted3::instances);
}

TEST(RefPtr, ImplicitlyCast) {
  {
    RefPtr ptr = MakeRefCounted<RefCounted3>();
    ASSERT_EQ(1, RefCounted3::instances);
    ASSERT_EQ(1, RefCountedVirtual::instances);
    RefPtr<RefCountedVirtual> p1(ptr);
    ASSERT_EQ(1, RefCounted3::instances);
    ASSERT_EQ(1, RefCountedVirtual::instances);
    RefPtr<RefCountedVirtual> p2(std::move(ptr));
    ASSERT_EQ(1, RefCounted3::instances);
    ASSERT_EQ(1, RefCountedVirtual::instances);
  }
  ASSERT_EQ(0, RefCounted3::instances);
  ASSERT_EQ(0, RefCountedVirtual::instances);
}

TEST(RefPtr, CopyFromNull) {
  RefPtr<RefCounted1> p1, p2;
  p1 = p2;
  // Shouldn't crash.
}

TEST(RefPtr, MoveFromNull) {
  RefPtr<RefCounted1> p1, p2;
  p1 = std::move(p2);
  // Shouldn't crash.
}

}  // namespace trpc
