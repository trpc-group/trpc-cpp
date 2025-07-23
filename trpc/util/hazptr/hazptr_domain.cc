//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/hazptr/hazptr_domain.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/hazptr/hazptr_domain.h"

#include <chrono>
#include <memory>

#include "trpc/util/hazptr/entry.h"
#include "trpc/util/hazptr/hazptr_object.h"
#include "trpc/util/internal/time_keeper.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/thread/internal/memory_barrier.h"

using namespace std::literals;

namespace trpc {

static constexpr uint32_t kRetireMask = 127;

HazptrDomain::HazptrDomain() {
  timer_id_ = internal::TimeKeeper::Instance()->AddTimer(
      {}, 100ms, [this](auto) { ReclaimBestEffort(); }, false);
}

HazptrDomain::~HazptrDomain() { internal::TimeKeeper::Instance()->KillTimer(timer_id_); }

hazptr::Entry* HazptrDomain::GetEntry() {
  auto p = hazptrs_.load(std::memory_order_acquire);
  while (p) {
    if (p->TryAcquire()) {
      return p;  // Kept in the list for later examination (during resource
                 // reclamation.).
    }
    p = p->next;
  }
  return GetEntrySlow();
}

void HazptrDomain::PutEntry(hazptr::Entry* entry) {
  entry->Release();
  // Kept for reuse.
}

void HazptrDomain::Retire(hazptr::Object* object) {
  PushRetired(object);

  auto old = retire_call_count_.fetch_add(1, std::memory_order_relaxed);

  if ((old & kRetireMask) == 0) {
    ReclaimBestEffort();
  }
}

void HazptrDomain::Reclaim(hazptr::Object* object) {
  PushRetired(object);

  ReclaimBestEffort();
}

hazptr::Entry* HazptrDomain::GetEntrySlow() {
  auto ptr = std::make_unique<hazptr::Entry>();
  auto head = hazptrs_.load(std::memory_order_relaxed);
  TRPC_CHECK(ptr->TryAcquire());
  ptr->domain = this;
  do {
    ptr->next = head;
  } while (!hazptrs_.compare_exchange_weak(head, ptr.get(), std::memory_order_release, std::memory_order_relaxed));
  return ptr.release();
}

void HazptrDomain::PushRetired(hazptr::Object* object) {
  auto head = retired_.load(std::memory_order_relaxed);
  do {
    object->next_ = head;
  } while (!retired_.compare_exchange_weak(head, object, std::memory_order_release, std::memory_order_relaxed));
}

void HazptrDomain::ReclaimBestEffort() {
  auto current = retired_.exchange(nullptr, std::memory_order_acquire);
  if (!current) {
    return;  // Someone else grabbed the list.
  }
  auto kept = GetKeptPointers();
  while (current) {
    auto next = current->next_;
    if (kept.find(current) != kept.end()) {
      PushRetired(current);  // It's still referenced by someone, try it next
                             // round.
    } else {
      current->DestroySelf();
    }
    current = next;
  }
}

std::unordered_set<const hazptr::Object*> HazptrDomain::GetKeptPointers() {
  std::unordered_set<const hazptr::Object*> objects;
  // Pairs with the light barrier in `Hazptr::TryKeep(...)`.
  internal::AsymmetricBarrierHeavy();
  // Surely there can be new `Entry` instances be inserted into `hazptrs_` when
  // we scan the list. That's safe, though. The reason is that by the time
  // `Retire` is called, we require the user to guarantee that no *more*
  // references can be made. Therefore, even if new `Entry` is created, it can't
  // points to whatever stored in `retired_` (i.e., what we're going to
  // destroyed), so we're safe here.
  auto current = hazptrs_.load(std::memory_order_acquire);
  while (current) {
    // The same argument as above, even if the entry changes from inactive to
    // active by the time (or soon after) we checked it, it won't hold a pointer
    // we're going to reclaim, so we're safe.
    if (current->active.load(std::memory_order_acquire)) {
      // Even if `TryGetPtr()` returns `nullptr`, it does no harm.
      objects.insert(current->TryGetPtr());
    }
    current = current->next;
  }
  return objects;
}

}  // namespace trpc
