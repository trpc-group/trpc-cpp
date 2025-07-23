//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/hazptr/entry_cache.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/check.h"
#include "trpc/util/hazptr/entry.h"
#include "trpc/util/hazptr/hazptr_domain.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::hazptr {

/// @brief Get `entry` from default domain
/// @return Entry*
Entry* GetEntryOfDefaultDomain() noexcept;

/// @brief Put `entry` to default domain
/// @param entry the pointer of `Entry` needed to put
void PutEntryOfDefaultDomain(Entry* entry) noexcept;

/// @brief Placeholder to mark the default domain. Recognized by this file only.
/// @note The reason why we do not use `GetDefaultHazptrDomain()` is for perf. reasons,
///       calling that method incurs some unnecessary overhead in fast path.
inline HazptrDomain* const kDefaultDomainPlaceholder = reinterpret_cast<HazptrDomain*>(1);

/// @brief Allocating `Entry` from `HazptrDomain` incurs some overhead, so we keep some
///       `Entry` locally to speed up allocation (in trade of slower reclamation path.)
///
/// @note Here we initialize all fields to `nullptr`, which would cause both `Get()`
///       and `Put()` to fall to slow path (as desired.). Besides, using `nullptr` here
///       permits static initialization of TLS.
struct EntryCache {
  // Note that `nullptr` would cause both `Get()` and `Put()` to fall to slow
  // path, which is desired.
  Entry** current = nullptr;
  Entry** bottom = nullptr;
  Entry** top = nullptr;

  /// @brief Get pointer of `Entry`
  /// @return Entry*
  Entry* Get() noexcept {
    if (TRPC_LIKELY(current > bottom)) {
      return *--current;
    }
    return GetSlow();
  }

  /// @brief Put `entry` to EntryCache
  /// @param entry the pointer of `Entry` needed to put
  void Put(Entry* entry) noexcept {
    if (TRPC_LIKELY(current < top)) {
      *current++ = entry;
      return;
    }
    PutSlow(entry);
  }

 private:
  void InitializeOnceCheck();

  Entry* GetSlow() noexcept;
  void PutSlow(Entry* entry) noexcept;
};

/// @brief Get default `EntryCache`
/// @return EntryCache*
inline EntryCache* GetEntryCacheOfDefaultDomain() noexcept {
  thread_local EntryCache entry_cache;
  return &entry_cache;
}

/// @brief For default domain, this method tries thread-local cache first, and fallback
///        to allocating from the `domain` if either the requested domain is not the
///        default one, or the cache is empty.
/// @param domain the  pointer of `HazptrDomain` which saved some `Entry`
/// @return Entry*
inline Entry* GetEntryOf(HazptrDomain* domain) noexcept {
  if (TRPC_LIKELY(domain == kDefaultDomainPlaceholder)) {
    return GetEntryCacheOfDefaultDomain()->Get();
  }
  return domain->GetEntry();
}

/// @brief Put `entry` to `domain`
/// @param domain the pointer of `HazptrDomain` which cached some `Entry`
/// @param entry the pointer of `Entry` is put to `domain`
inline void PutEntryOf(HazptrDomain* domain, Entry* entry) noexcept {
  TRPC_DCHECK(entry->TryGetPtr() == nullptr);
  if (TRPC_LIKELY(domain == kDefaultDomainPlaceholder)) {
    return GetEntryCacheOfDefaultDomain()->Put(entry);
  }
  return domain->PutEntry(entry);
}

}  // namespace trpc::hazptr
