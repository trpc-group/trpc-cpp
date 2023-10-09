//
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/casting_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/internal/casting.h"

#include <iostream>
#include <memory>

#include "gtest/gtest.h"

namespace trpc::internal::testing {

struct Base {
  enum { kA, kB, kC } type;
};

struct A : Base {
  A() { type = kA; }

  static bool classof(const Base& val) { return val.type == kA || val.type == kB; }
};

struct B : A {
  B() { type = kB; }

  static bool classof(const Base& val) { return val.type == kB; }
};

struct C : Base {
  C() { type = kC; }

  static bool classof(const Base& val) { return val.type == kC; }
};

struct C1 : ExactMatchCastable {};

struct C2 : C1 {
  C2() { SetRuntimeType(this, kRuntimeType<C2>); }
};

struct C3 : C1 {
  C3() { SetRuntimeType(this, kRuntimeType<C3>); }
};

TEST(Casting, Nullptr) {
  B* pb = nullptr;
  std::cout << "Nullptr" << std::endl;
  ASSERT_EQ(nullptr, dyn_cast_or_null<A>(pb));
  ASSERT_EQ(nullptr, cast_or_null<A>(pb));
}

TEST(Casting, DownCastFailure) {
  auto pa = std::make_unique<A>();
  std::cout << "DownCastFailure" << std::endl;
  ASSERT_EQ(nullptr, dyn_cast<B>(pa.get()));
}

TEST(Casting, Cast) {
  auto pb = std::make_unique<B>();
  Base* ptr = pb.get();

  ASSERT_NE(nullptr, dyn_cast<Base>(ptr));
  ASSERT_NE(nullptr, dyn_cast<A>(ptr));
  ASSERT_NE(nullptr, dyn_cast<B>(ptr));

  ASSERT_NE(nullptr, cast<A>(ptr));   // Casting pointer.
  ASSERT_NE(nullptr, cast<B>(ptr));   // Casting pointer.
  ASSERT_NE(nullptr, cast<A>(*ptr));  // Casting pointer.
  ASSERT_NE(nullptr, cast<B>(*ptr));  // Casting pointer.

  std::cout << "Cast" << std::endl;

  ASSERT_NE(nullptr, dyn_cast<B>(pb.get()));  // Cast to self.
  ASSERT_EQ(nullptr, dyn_cast<C>(ptr));       // Cast failure.
}

TEST(Casting, CastWithConst) {
  auto pb = std::make_unique<const B>();
  const Base* ptr = pb.get();

  ASSERT_NE(nullptr, dyn_cast<Base>(ptr));
  ASSERT_NE(nullptr, dyn_cast<A>(ptr));
  ASSERT_NE(nullptr, dyn_cast<B>(ptr));

  ASSERT_NE(nullptr, cast<A>(ptr));   // Casting pointer.
  ASSERT_NE(nullptr, cast<B>(ptr));   // Casting pointer.
  ASSERT_NE(nullptr, cast<A>(*ptr));  // Casting pointer.
  ASSERT_NE(nullptr, cast<B>(*ptr));  // Casting pointer.

  std::cout << "CastWithConst" << std::endl;

  ASSERT_NE(nullptr, dyn_cast<B>(pb.get()));  // Cast to self.
  ASSERT_EQ(nullptr, dyn_cast<C>(ptr));       // Cast failure.
}

TEST(Casting, ExactMatchCastable) {
  auto pc2 = std::make_unique<C2>();
  C1* p = pc2.get();

  std::cout << "ExactMatchCastable" << std::endl;
  ASSERT_NE(nullptr, dyn_cast<C1>(p));  // Success.
  ASSERT_NE(nullptr, dyn_cast<C2>(p));  // Success.
  ASSERT_EQ(nullptr, dyn_cast<C3>(p));  // Failure.
}

}  // namespace trpc::internal::testing
