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

#include "trpc/util/object_pool/object_pool_ptr.h"

#include <cstdlib>
#include <string>
#include <utility>

#include "gtest/gtest.h"

namespace trpc::object_pool {

struct A {
  int a{};
  virtual ~A() = default;
};

template <>
struct ObjectPoolTraits<A> {
  static constexpr auto kType = ObjectPoolType::kGlobal;
};

struct B : public A {
  int b1{};
  std::string b2;
};

template <>
struct ObjectPoolTraits<B> {
  static constexpr auto kType = ObjectPoolType::kGlobal;
};

TEST(ObjectPoolPtrTest, LwUniqueTest) {
  trpc::object_pool::LwUniquePtr<B> ptr1 = trpc::object_pool::MakeLwUnique<B>();

  ASSERT_TRUE(ptr1 != nullptr);
  ASSERT_TRUE(nullptr != ptr1);

  trpc::object_pool::LwUniquePtr<B> ptr2 = trpc::object_pool::MakeLwUnique<B>();
  ptr2->a = 1;
  ptr2->b1 = 2;
  ptr2->b2 = "hello";

  ASSERT_TRUE(ptr2 != ptr1);

  trpc::object_pool::LwUniquePtr<B> ptr3(std::move(ptr2));

  ASSERT_TRUE(ptr2.Get() == nullptr);
  ASSERT_TRUE(ptr3.Get() != nullptr);
  ASSERT_TRUE(ptr3->a == 1);

  trpc::object_pool::LwUniquePtr<B> ptr4;
  ptr4 = std::move(ptr3);

  ASSERT_TRUE(ptr3.Get() == nullptr);
  ASSERT_TRUE(ptr4.Get() != nullptr);
  ASSERT_TRUE(ptr4->a == 1);

  ptr4.Reset();

  ASSERT_TRUE(ptr4.Get() == nullptr);
}

struct G : public LwSharedPtrCountBase {
  int a{};
  std::string s;
};

template <>
struct ObjectPoolTraits<G> {
  static constexpr auto kType = ObjectPoolType::kGlobal;
};

struct H : public EnableLwSharedFromThis<H> {
  int a{};
  std::string s;
};

template <>
struct ObjectPoolTraits<H> {
  static constexpr auto kType = ObjectPoolType::kGlobal;
};

TEST(ObjectPoolPtrTest, LwPtrSharedTest) {
  trpc::object_pool::LwSharedPtr<G> ptr1 = trpc::object_pool::MakeLwShared<G>();

  ASSERT_TRUE(ptr1 != nullptr);
  ASSERT_TRUE(nullptr != ptr1);
  ASSERT_TRUE(ptr1.UseCount() == 1);

  trpc::object_pool::LwSharedPtr<G> ptr2 = trpc::object_pool::MakeLwShared<G>();

  ASSERT_TRUE(ptr2 != ptr1);

  ptr2->a = 1;
  ptr2->s = "hello";

  trpc::object_pool::LwSharedPtr<G> ptr3(std::move(ptr2));

  ASSERT_TRUE(ptr2.Get() == nullptr);
  ASSERT_TRUE(ptr3.Get() != nullptr);
  ASSERT_TRUE(ptr3.UseCount() == 1);
  ASSERT_TRUE(ptr3->a == 1);

  trpc::object_pool::LwSharedPtr<G> ptr4;
  ptr4 = std::move(ptr3);

  ASSERT_TRUE(ptr3.Get() == nullptr);
  ASSERT_TRUE(ptr4.Get() != nullptr);
  ASSERT_TRUE(ptr4.UseCount() == 1);

  trpc::object_pool::LwSharedPtr<G> ptr5;
  ptr5 = ptr4;

  ASSERT_TRUE(ptr4.Get() != nullptr);
  ASSERT_TRUE(ptr5.Get() != nullptr);
  ASSERT_TRUE(ptr4.UseCount() == 2);
  ASSERT_TRUE(ptr5.UseCount() == 2);

  trpc::object_pool::LwSharedPtr<G> ptr6(ptr5);

  ASSERT_TRUE(ptr5.Get() != nullptr);
  ASSERT_TRUE(ptr6.Get() != nullptr);
  ASSERT_TRUE(ptr5.UseCount() == 3);
  ASSERT_TRUE(ptr6.UseCount() == 3);

  trpc::object_pool::LwSharedPtr<H> ptr7 = trpc::object_pool::MakeLwShared<H>();

  ASSERT_TRUE(ptr7 != nullptr);

  trpc::object_pool::LwSharedPtr<H> ptr8 = ptr7->SharedFromThis();

  ASSERT_TRUE(ptr7.UseCount() == 2);
  ASSERT_TRUE(ptr8.UseCount() == 2);
  ASSERT_TRUE(ptr7.Get() == ptr8.Get());
}

// lw_shared_ref_ptr
TEST(ObjectPoolPtrTest, lw_shared_ref_ptr_constructor) {
  trpc::object_pool::LwSharedPtr<H> ptr1 = trpc::object_pool::MakeLwShared<H>();
  trpc::object_pool::LwSharedPtr<H> ptr2(object_pool::lw_shared_ref_ptr, ptr1.Get());
  ASSERT_EQ(ptr1.Get(), ptr2.Get());
  ASSERT_EQ(ptr1.UseCount(), ptr2.UseCount());
}
TEST(ObjectPoolPtrTest, lw_shared_ref_ptr_reset) {
  trpc::object_pool::LwSharedPtr<H> ptr1 = trpc::object_pool::MakeLwShared<H>();
  trpc::object_pool::LwSharedPtr<H> ptr2;
  ptr2.Reset(object_pool::lw_shared_ref_ptr, ptr1.Get());
  ASSERT_EQ(ptr1.Get(), ptr2.Get());
  ASSERT_EQ(ptr1.UseCount(), ptr2.UseCount());
}

// lw_shared_adopt_ptr
TEST(ObjectPoolPtrTest, lw_shared_adopt_ptr_constructor) {
  trpc::object_pool::LwSharedPtr<H> ptr1 = trpc::object_pool::MakeLwShared<H>();
  auto* p = ptr1.Leak();
  ASSERT_TRUE(p);
  ASSERT_TRUE(!ptr1.Get());
  ASSERT_EQ(ptr1.UseCount(), 0);

  trpc::object_pool::LwSharedPtr<H> ptr2(object_pool::lw_shared_adopt_ptr, p);
  ASSERT_TRUE(ptr2.Get());
  ASSERT_EQ(ptr2.UseCount(), 1);
}
TEST(ObjectPoolPtrTest, lw_shared_adopt_ptr_reset) {
  trpc::object_pool::LwSharedPtr<H> ptr1 = trpc::object_pool::MakeLwShared<H>();
  auto* p = ptr1.Leak();
  ASSERT_TRUE(p);
  ASSERT_TRUE(!ptr1.Get());
  ASSERT_EQ(ptr1.UseCount(), 0);

  trpc::object_pool::LwSharedPtr<H> ptr2;
  ptr2.Reset(object_pool::lw_shared_adopt_ptr, p);
  ASSERT_TRUE(ptr2.Get());
  ASSERT_EQ(ptr2.UseCount(), 1);
}

TEST(ObjectPoolPtrTest, lw_shared_incrcount) {
  H* p = nullptr;
  {
    trpc::object_pool::LwSharedPtr<H> ptr1 = trpc::object_pool::MakeLwShared<H>();
    p = ptr1.Get();
    ASSERT_EQ(p->UseCount(), 1);
    p->IncrCount();
    ASSERT_EQ(p->UseCount(), 2);
  }
  ASSERT_EQ(p->UseCount(), 1);
  p->DecrCount();
  ASSERT_EQ(p->UseCount(), 0);
}

TEST(ObjectPoolPtrTest, UniqueTest) {
  trpc::object_pool::UniquePtr<B> ptr1 = trpc::object_pool::MakeUnique<B>();

  ASSERT_TRUE(ptr1 != nullptr);
  ASSERT_TRUE(nullptr != ptr1);

  trpc::object_pool::UniquePtr<B> ptr2 = trpc::object_pool::MakeUnique<B>();
  ptr2->a = 1;
  ptr2->b1 = 2;
  ptr2->b2 = "hello";

  ASSERT_TRUE(ptr2 != ptr1);

  // test polymorphism
  trpc::object_pool::UniquePtr<A> ptr3(std::move(ptr2));

  ASSERT_TRUE(ptr2.Get() == nullptr);
  ASSERT_TRUE(ptr3.Get() != nullptr);
  ASSERT_TRUE(ptr3->a == 1);

  trpc::object_pool::UniquePtr<B> ptr4;
  ptr4 = std::move(ptr3);

  ASSERT_TRUE(ptr3.Get() == nullptr);
  ASSERT_TRUE(ptr4.Get() != nullptr);
  ASSERT_TRUE(ptr4->a == 1);

  ptr4.Reset();

  ASSERT_TRUE(ptr4.Get() == nullptr);
}

struct C : public SharedPtrCountBase {
  int a{};
  std::string s;
};

template <>
struct ObjectPoolTraits<C> {
  static constexpr auto kType = ObjectPoolType::kGlobal;
};

struct D : public EnableSharedFromThis<D> {
  int a{};
  std::string s;
};

template <>
struct ObjectPoolTraits<D> {
  static constexpr auto kType = ObjectPoolType::kGlobal;
};

TEST(ObjectPoolPtrTest, ObjectPoolPtrSharedTest) {
  trpc::object_pool::SharedPtr<C> ptr1 = trpc::object_pool::MakeShared<C>();

  ASSERT_TRUE(ptr1 != nullptr);
  ASSERT_TRUE(nullptr != ptr1);
  ASSERT_TRUE(ptr1.UseCount() == 1);

  trpc::object_pool::SharedPtr<C> ptr2 = trpc::object_pool::MakeShared<C>();

  ASSERT_TRUE(ptr2 != ptr1);

  ptr2->a = 1;
  ptr2->s = "hello";

  trpc::object_pool::SharedPtr<C> ptr3(std::move(ptr2));

  ASSERT_TRUE(ptr2.Get() == nullptr);
  ASSERT_TRUE(ptr3.Get() != nullptr);
  ASSERT_TRUE(ptr3.UseCount() == 1);
  ASSERT_TRUE(ptr3->a == 1);

  trpc::object_pool::SharedPtr<C> ptr4;
  ptr4 = std::move(ptr3);

  ASSERT_TRUE(ptr3.Get() == nullptr);
  ASSERT_TRUE(ptr4.Get() != nullptr);
  ASSERT_TRUE(ptr4.UseCount() == 1);

  trpc::object_pool::SharedPtr<C> ptr5;
  ptr5 = ptr4;

  ASSERT_TRUE(ptr4.Get() != nullptr);
  ASSERT_TRUE(ptr5.Get() != nullptr);
  ASSERT_TRUE(ptr4.UseCount() == 2);
  ASSERT_TRUE(ptr5.UseCount() == 2);

  trpc::object_pool::SharedPtr<C> ptr6(ptr5);

  ASSERT_TRUE(ptr5.Get() != nullptr);
  ASSERT_TRUE(ptr6.Get() != nullptr);
  ASSERT_TRUE(ptr5.UseCount() == 3);
  ASSERT_TRUE(ptr6.UseCount() == 3);

  trpc::object_pool::SharedPtr<D> ptr7 = trpc::object_pool::MakeShared<D>();

  ASSERT_TRUE(ptr7 != nullptr);

  trpc::object_pool::SharedPtr<D> ptr8 = ptr7->SharedFromThis();

  ASSERT_TRUE(ptr7.UseCount() == 2);
  ASSERT_TRUE(ptr8.UseCount() == 2);
  ASSERT_TRUE(ptr7.Get() == ptr8.Get());
}

// shared_ref_ptr
TEST(ObjectPoolPtrTest, shared_ref_ptr_constructor) {
  trpc::object_pool::SharedPtr<D> ptr1 = trpc::object_pool::MakeShared<D>();
  trpc::object_pool::SharedPtr<D> ptr2(object_pool::shared_ref_ptr, ptr1.Get());
  ASSERT_EQ(ptr1.Get(), ptr2.Get());
  ASSERT_EQ(ptr1.UseCount(), ptr2.UseCount());
}
TEST(ObjectPoolPtrTest, shared_ref_ptr_reset) {
  trpc::object_pool::SharedPtr<D> ptr1 = trpc::object_pool::MakeShared<D>();
  trpc::object_pool::SharedPtr<D> ptr2;
  ptr2.Reset(object_pool::shared_ref_ptr, ptr1.Get());
  ASSERT_EQ(ptr1.Get(), ptr2.Get());
  ASSERT_EQ(ptr1.UseCount(), ptr2.UseCount());
}

// shared_adopt_ptr
TEST(ObjectPoolPtrTest, shared_adopt_ptr_constructor) {
  trpc::object_pool::SharedPtr<D> ptr1 = trpc::object_pool::MakeShared<D>();
  auto* p = ptr1.Leak();
  ASSERT_TRUE(p);
  ASSERT_TRUE(!ptr1.Get());
  ASSERT_EQ(ptr1.UseCount(), 0);

  trpc::object_pool::SharedPtr<D> ptr2(object_pool::shared_adopt_ptr, p);
  ASSERT_TRUE(ptr2.Get());
  ASSERT_EQ(ptr2.UseCount(), 1);
}
TEST(ObjectPoolPtrTest, shared_adopt_ptr_reset) {
  trpc::object_pool::SharedPtr<D> ptr1 = trpc::object_pool::MakeShared<D>();
  auto* p = ptr1.Leak();
  ASSERT_TRUE(p);
  ASSERT_TRUE(!ptr1.Get());
  ASSERT_EQ(ptr1.UseCount(), 0);

  trpc::object_pool::SharedPtr<D> ptr2;
  ptr2.Reset(object_pool::shared_adopt_ptr, p);
  ASSERT_TRUE(ptr2.Get());
  ASSERT_EQ(ptr2.UseCount(), 1);
}

TEST(ObjectPoolPtrTest, shared_incrcount) {
  D* p = nullptr;
  {
    trpc::object_pool::SharedPtr<D> ptr1 = trpc::object_pool::MakeShared<D>();
    p = ptr1.Get();
    ASSERT_EQ(p->UseCount(), 1);
    p->IncrCount();
    ASSERT_EQ(p->UseCount(), 2);
  }
  ASSERT_EQ(p->UseCount(), 1);
  p->DecrCount();
  ASSERT_EQ(p->UseCount(), 0);
}

struct E : public EnableSharedFromThis<E> {
  int a{};
  std::string s;
};

template <>
struct ObjectPoolTraits<E> {
  static constexpr auto kType = ObjectPoolType::kGlobal;
};

struct F : public E {
  int b{};
};

template <>
struct ObjectPoolTraits<F> {
  static constexpr auto kType = ObjectPoolType::kGlobal;
};

TEST(ObjectPoolPtrTest, SharedPolymorphismTest) {
  trpc::object_pool::SharedPtr<F> ptr1 = trpc::object_pool::MakeShared<F>();

  ASSERT_TRUE(ptr1 != nullptr);

  trpc::object_pool::SharedPtr<F> ptr2 = StaticPointerCast<F, E>(ptr1->SharedFromThis());

  ASSERT_TRUE(ptr1.UseCount() == 2);
  ASSERT_TRUE(ptr2.UseCount() == 2);
  ASSERT_TRUE(ptr1.Get() == ptr2.Get());

  trpc::object_pool::SharedPtr<E> ptr3 = ptr1->SharedFromThis();

  ASSERT_TRUE(ptr1.UseCount() == 3);
  ASSERT_TRUE(ptr3.UseCount() == 3);
  ASSERT_TRUE(ptr1.Get() == ptr3.Get());
}

}  // namespace trpc::object_pool
