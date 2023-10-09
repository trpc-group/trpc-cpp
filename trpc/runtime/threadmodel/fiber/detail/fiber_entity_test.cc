//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/fiber_entity_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/threadmodel/fiber/detail/fiber_entity.h"

#include "gtest/gtest.h"

namespace trpc::fiber::detail {

TEST(FiberEntity, GetMaster) {
  SetUpMasterFiberEntity();
  auto* fiber = GetMasterFiberEntity();
  ASSERT_TRUE(!!fiber);
  // What can we test here?
}

TEST(FiberEntity, CreateDestroy) {
  auto fiber = CreateFiberEntity(nullptr, [] {});
  ASSERT_TRUE(!!fiber);
  FreeFiberEntity(fiber);
}

TEST(FiberEntity, GetStackTop) {
  auto fiber = CreateFiberEntity(nullptr, [] {});
  ASSERT_TRUE(fiber->GetStackTop());
  FreeFiberEntity(fiber);
}

FiberEntity* master;

TEST(FiberEntity, Switch) {
  master = GetMasterFiberEntity();
  int x = 0;
  auto fiber = CreateFiberEntity(nullptr, [&] {
    x = 10;
    // Jump back to the master fiber.
    master->Resume();
  });
  fiber->Resume();

  // Back from `cb`.
  ASSERT_EQ(10, x);
  FreeFiberEntity(fiber);
}

TEST(FiberEntity, GetCurrent) {
  master = GetMasterFiberEntity();
  ASSERT_EQ(master, GetCurrentFiberEntity());

  FiberEntity* ptr;
  auto fiber = CreateFiberEntity(nullptr, [&] {
    ASSERT_EQ(GetCurrentFiberEntity(), ptr);
    GetMasterFiberEntity()->Resume();  // Equivalent to `master->Resume().`
  });
  ptr = fiber;
  fiber->Resume();

  ASSERT_EQ(master, GetCurrentFiberEntity());  // We're back.
  FreeFiberEntity(fiber);
}

TEST(FiberEntity, ResumeOn) {
  struct Context {
    FiberEntity* expected;
    bool tested = false;
  };
  bool fiber_run = false;
  auto fiber = CreateFiberEntity(nullptr, [&] {
    GetMasterFiberEntity()->Resume();
    fiber_run = true;
    GetMasterFiberEntity()->Resume();
  });

  Context ctx;
  ctx.expected = fiber;
  fiber->Resume();  // We (master fiber) will be immediately resumed by
                    // `fiber_cb`.
  fiber->ResumeOn([&] {
    ASSERT_EQ(GetCurrentFiberEntity(), ctx.expected);
    ctx.tested = true;
  });

  ASSERT_TRUE(ctx.tested);
  ASSERT_TRUE(fiber_run);
  ASSERT_EQ(master, GetCurrentFiberEntity());
  FreeFiberEntity(fiber);
}

TEST(FiberEntity, Fls) {
  static const std::size_t kSlots[] = {
      0,
      1,
      FiberEntity::kInlineLocalStorageSlots + 5,
      FiberEntity::kInlineLocalStorageSlots + 9999,
  };

  for (auto slot_index : kSlots) {
    auto self = GetCurrentFiberEntity();
    *self->GetFls(slot_index) = MakeErased<int>(5);

    bool fiber_run = false;
    auto fiber = CreateFiberEntity(nullptr, [&] {
      auto self = GetCurrentFiberEntity();
      auto fls = self->GetFls(slot_index);
      ASSERT_FALSE(*fls);

      GetMasterFiberEntity()->Resume();

      ASSERT_EQ(fls, self->GetFls(slot_index));
      *fls = MakeErased<int>(10);

      GetMasterFiberEntity()->Resume();

      ASSERT_EQ(fls, self->GetFls(slot_index));
      ASSERT_EQ(10, *reinterpret_cast<int*>(fls->Get()));
      fiber_run = true;

      GetMasterFiberEntity()->Resume();
    });

    ASSERT_EQ(self, GetMasterFiberEntity());
    auto fls = self->GetFls(slot_index);
    ASSERT_EQ(5, *reinterpret_cast<int*>(fls->Get()));

    fiber->Resume();

    ASSERT_EQ(fls, self->GetFls(slot_index));

    fiber->Resume();

    ASSERT_EQ(5, *reinterpret_cast<int*>(fls->Get()));
    ASSERT_EQ(fls, self->GetFls(slot_index));
    *reinterpret_cast<int*>(fls->Get()) = 7;

    fiber->Resume();

    ASSERT_EQ(7, *reinterpret_cast<int*>(fls->Get()));
    ASSERT_EQ(fls, self->GetFls(slot_index));

    ASSERT_TRUE(fiber_run);
    ASSERT_EQ(master, GetCurrentFiberEntity());
    FreeFiberEntity(fiber);
  }
}

TEST(FiberEntity, ResumeOnMaster) {
  struct Context {
    FiberEntity* expected;
    bool tested = false;
  };
  Context ctx;
  auto fiber = CreateFiberEntity(nullptr, [&] {
    master->ResumeOn([&] {
      ASSERT_EQ(GetCurrentFiberEntity(), ctx.expected);
      ctx.tested = true;
      // Continue running master fiber on return.
    });
  });

  ctx.expected = GetMasterFiberEntity();
  fiber->Resume();

  ASSERT_TRUE(ctx.tested);
  ASSERT_EQ(master, GetCurrentFiberEntity());
  FreeFiberEntity(fiber);
}
TEST(FiberEntity, List) {
  auto fiber = CreateFiberEntity(nullptr, [] {});
  ASSERT_TRUE(!!fiber);
  FreeFiberEntity(fiber);
  ASSERT_EQ(&fiber->chain, fiber->chain.next);
  ASSERT_EQ(&fiber->chain, fiber->chain.prev);

  FiberEntityList list;
  auto fiber1 = CreateFiberEntity(nullptr, [] {});
  list.push_back(fiber1);
  ASSERT_EQ(list.size(), 1);
  list.pop_front();
  ASSERT_EQ(list.size(), 0);

  ASSERT_EQ(&fiber1->chain, fiber1->chain.next);
  ASSERT_EQ(&fiber1->chain, fiber1->chain.prev);
  FreeFiberEntity(fiber1);
  ASSERT_EQ(&fiber1->chain, fiber1->chain.next);
  ASSERT_EQ(&fiber1->chain, fiber1->chain.prev);
}

}  // namespace trpc::fiber::detail
