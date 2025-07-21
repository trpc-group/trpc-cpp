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

#include "trpc/util/buffer/memory_pool/global_memory_pool.h"

#include <sys/mman.h>
#include <unistd.h>

#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>

#include "trpc/util/buffer/memory_pool/common.h"
#include "trpc/util/check.h"
#include "trpc/util/internal/never_destroyed.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::memory_pool::global {

namespace detail {

static constexpr std::size_t kBlockNum = 32;    // The number of Block objects contained in a BlockList.
static constexpr uint32_t kBlockListSize = 32;  // The number of BlockList objects contained in a `BlockChunk`.
// The total number of Block objects contained in a `BlockChunk`.
static constexpr std::size_t kChunkBlockNum = kBlockListSize * kBlockNum;

static constexpr std::size_t kFreeBlockNum = 32;  // The length of the reclaimed free list.
static constexpr std::size_t kGlobalPoolNum = 1;  // The number of `GlobalMemPool`

/// @brief Get the number of Block objects based on the memory threshold and the set Block size:
/// @return Return the maximum number of Block.
std::size_t GetMaxBlockNumByPoolThreshold() { return GetMemPoolThreshold() / GetMemBlockSize(); }

/// @brief Allocate a Block pointer.
/// @param need_free_to_system Memory deallocation needs to be returned to the system.
/// @return Block pointer
static Block* AllocateBlock(bool need_free_to_system) {
  std::size_t block_size = GetMemBlockSize();
  TRPC_ASSERT(block_size > 64 && "block must bigger than 64.");
  AllocateMemFunc allocate_func = GetAllocateMemFunc();
  TRPC_ASSERT(allocate_func);
  char* addr = reinterpret_cast<char*>(allocate_func(alignof(Block), block_size));
  if (TRPC_UNLIKELY(!addr)) {
    return nullptr;
  }
  Block* block = new (addr)
      Block{.next = nullptr, .ref_count = 1, .need_free_to_system = need_free_to_system, .data = addr + sizeof(Block)};
  return block;
}

/// @brief Freeing a memory block.
/// @param block The Block pointer that needs to be freed.
static void DellocateBlock(Block* block) {
  DeallocateMemFunc del_fun = GetDeallocateMemFunc();
  TRPC_ASSERT(del_fun);
  del_fun(static_cast<void*>(block));
}

/// @brief The data structure for storing a linked list of multiple Block objects.
struct alignas(64) BlockList {
  // A pointer array for storing blocks, where each object in the array is the head of a linked list.
  Block* blocks[kBlockNum] = {nullptr};
  // The minimum index in `blocks` where an available Block object can be obtained.
  size_t idx{0};
};

/// @brief The data structure for storing multiple BlockList objects.
struct alignas(64) BlockChunk {
  BlockList block_list[kBlockListSize];  ///< An array for storing BlockList objects.
  size_t idx{0};  ///< The minimum index in `block_list` where an available BlockList object can be obtained.
};

/// @brief A recycling linked list for Block objects, making it easier to retrieve them next time.
struct FreeBlockList {
  Block* head{nullptr};  ///< The pointer to the head of a linked list.
  size_t length{0};      ///< The length of a linked list.
};

/// @brief A global memory pool object that stores all Block objects, which is a shared resource pool for multiple
///        thread memory pools. All Block objects in the local TLS memory pool of each thread are sourced from
///        instances of this class.
/// @note  Multiple instances of this class can exist, but the default is one.
class alignas(64) GlobalMemPool {
  public:
  GlobalMemPool() noexcept;
  ~GlobalMemPool();

  /// @brief Recycling a FreeBlockList object
  /// @param [in] free_block_list Reference to a FreeBlockList object.
  void PushFreeBlockList(FreeBlockList& free_block_list) noexcept;

  /// @brief Allocate a FreeBlockList object to the caller.
  /// @param [out] free_block_list Reference to a FreeBlockList object.
  /// @return A boolean type, true: success; false: failure.
  bool PopFreeBlockList(FreeBlockList& free_block_list) noexcept;

  /// @brief Allocate a BlockList object to the caller.
  /// @return BlockList pointer
  BlockList* PopBlockList() noexcept;

 private:
  // Bulk request Block objects from the system.
  bool NewBlockChunk() noexcept;

 private:
  size_t max_block_num_{0};
  // Block management unit.
  struct BlockManager {
    size_t available_size{0};  // The number of available BlockChunks.
    std::vector<BlockChunk> block_chunks;
  };
  BlockManager block_manager_;

  // Free list management unit.
  struct BlockFreeListManager {
    size_t free_num{0};
    std::vector<FreeBlockList> free_block_vec;
  };
  BlockFreeListManager free_block_manager_;

  size_t alloc_block_num_{0};
  std::mutex block_mutex_;
  std::mutex free_block_mutex_;
};

GlobalMemPool::GlobalMemPool() noexcept {
  max_block_num_ = GetMaxBlockNumByPoolThreshold();

  size_t block_chunks_size = (max_block_num_ + kChunkBlockNum - 1) / kChunkBlockNum;
  TRPC_ASSERT(block_chunks_size > 0 && "`mem threshold is too small`");

  block_manager_.block_chunks.resize(block_chunks_size * 2);
  block_manager_.available_size = 0;

  size_t free_block_vec_size = (max_block_num_ + kFreeBlockNum - 1) / kFreeBlockNum;
  free_block_manager_.free_block_vec.resize(free_block_vec_size * 2);
  free_block_manager_.free_num = 0;

  // Allocate a `BlockChunk` during initialization.
  if (!NewBlockChunk()) {
    TRPC_LOG_ERROR("GlobalMemPool NewBlockChunk failed.");
    exit(-1);
  }
}

GlobalMemPool::~GlobalMemPool() {
  for (size_t i = 0; i < block_manager_.available_size; ++i) {
    for (size_t j = 0; j < kBlockListSize; ++j) {
      for (size_t k = 0; k < kBlockNum; ++k) {
        Block* block = block_manager_.block_chunks[i].block_list[j].blocks[k];
        if (block != nullptr) {
          DellocateBlock(block);
        }
      }
    }
  }
}

void GlobalMemPool::PushFreeBlockList(FreeBlockList& free_block_list) noexcept {
  {
    std::unique_lock<std::mutex> lock(free_block_mutex_);
    // Record the head pointer of the free list in the `free_block_manager_`.
    free_block_manager_.free_block_vec[free_block_manager_.free_num++] = free_block_list;
  }
  // Reset the free list `free_block_list`.
  free_block_list.head = nullptr;
  free_block_list.length = 0;
}

bool GlobalMemPool::PopFreeBlockList(FreeBlockList& free_block_list) noexcept {
  std::unique_lock<std::mutex> lock(free_block_mutex_);
  // If there is an available free list, allocate one directly from the free list.
  if (free_block_manager_.free_num > 0) {
    free_block_list = free_block_manager_.free_block_vec[--free_block_manager_.free_num];
    lock.unlock();
    return true;
  }

  return false;
}

// Allocate space.
bool GlobalMemPool::NewBlockChunk() noexcept {
  if (TRPC_UNLIKELY(alloc_block_num_ >= max_block_num_)) {
    TRPC_FMT_INFO_EVERY_SECOND("Block Allocate {}, beyond {} limited.", alloc_block_num_, max_block_num_);
    return false;
  }

  BlockChunk* new_block_chunk = &(block_manager_.block_chunks[block_manager_.available_size]);

  new_block_chunk->idx = 0;  // Initialize `idx` in BlockChunk to 0, indicating that all blocks are available.
  for (uint32_t i = 0; i < kBlockListSize; ++i) {
    // Initialize the minimum available index of `blocks` in the BlockList object.
    new_block_chunk->block_list[i].idx = 0;
    for (uint32_t k = 0; k < kBlockNum; ++k) {
      Block* block = AllocateBlock(false);
      TRPC_ASSERT(block != nullptr);
      new_block_chunk->block_list[i].blocks[k] = block;
    }
  }

  ++(block_manager_.available_size);

  alloc_block_num_ += kChunkBlockNum;  // Accumulate the number of allocated Block objects.

  return true;
}

BlockList* GlobalMemPool::PopBlockList() noexcept {
  // `GlobalMemPool` will be shared by memory pools of multiple threads, so it needs to be protected by locks here.
  std::unique_lock<std::mutex> lock(block_mutex_);

  // Get the BlockChunk object that has already been allocated.
  BlockChunk* block_chunk = &(block_manager_.block_chunks[block_manager_.available_size - 1]);

  if (block_chunk->idx < kBlockListSize) {
    // If there are still available blocks in the `block_list` of the current `block_chunk`, get them directly from
    // there.
    auto res_idx = block_chunk->idx++;
    lock.unlock();
    return &block_chunk->block_list[res_idx];
  }

  // If the BlockList in the current `block_chunk` is exhausted, allocate a new BlockChunk object.
  if (TRPC_LIKELY(NewBlockChunk())) {
    // Get the latest allocated `block_chunk`.
    block_chunk = &(block_manager_.block_chunks[block_manager_.available_size - 1]);
    // Allocate the `block_list` in the `block_chunk`.
    auto res_idx = block_chunk->idx++;
    lock.unlock();
    return &block_chunk->block_list[res_idx];
  }
  // Return nullptr if the allocation fails.
  return nullptr;
}

/// @brief A memory pool class for local threads.
class alignas(64) LocalMemPool {
 public:
  explicit LocalMemPool(GlobalMemPool* global_pool) noexcept;
  ~LocalMemPool();

  /// @brief Allocate a Block object.
  /// @return Block pointer
  Block* Allocate() noexcept;

  /// @brief Free a Block object.
  /// @param block Pointer to the Block object that needs to be freed.
  void Deallocate(Block* block) noexcept;

  /// @brief Get and set statistical data related to the allocation and deallocation of Block objects.
  /// @return Reference to Statistics.
  inline Statistics& GetStatistics() { return stat_; }

  /// @brief Print statistical data related to the allocation and deallocation of Block objects.
  void PrintStatistics();

 private:
  // The corresponding global memory pool does not need to be freed, it will be automatically released when the current
  // thread exits.
  GlobalMemPool* global_pool_;

  // The BlockList obtained from the global memory pool does not need to be freed, its lifetime is managed by
  // `global_pool_`.
  BlockList* block_list_{nullptr};

  // Reclaim the free block linked list.
  FreeBlockList free_block_list_;

  // Statistics information.
  Statistics stat_;
};

LocalMemPool::LocalMemPool(GlobalMemPool* global_pool) noexcept : global_pool_(global_pool) {}

LocalMemPool::~LocalMemPool() {
  if (block_list_ != nullptr) {
    while (block_list_->idx < kBlockNum) {
      if (TRPC_UNLIKELY(free_block_list_.length == kFreeBlockNum)) {
        // If the batch recycling threshold is met, the blocks will be directly recycled to `global_pool_`.
        global_pool_->PushFreeBlockList(free_block_list_);
      }

      // Release the Block objects in `block_list_` to `free_block_list_` first.
      auto* block = block_list_->blocks[block_list_->idx++];
      block->next = free_block_list_.head;
      free_block_list_.head = block;
      ++free_block_list_.length;
    }
  }

  if (free_block_list_.length > 0) {
    // Return to `global_pool_`.
    global_pool_->PushFreeBlockList(free_block_list_);
  }
}

Block* LocalMemPool::Allocate() noexcept {
  Block* block = nullptr;
  ++GetStatistics().total_allocs_num;

  // If `free_block_list_` still has available space, directly get a Block object from it and give it to the caller.
  if (TRPC_LIKELY(free_block_list_.head != nullptr)) {
    ++GetStatistics().allocs_from_tls_free_list;

    block = free_block_list_.head;
    free_block_list_.head = block->next;
    --free_block_list_.length;

    return block;
  }

  // Get the free list `free_block_list_ `from global_pool_, and then get a Block object from `free_block_list_` and
  // give it to the caller.
  if (global_pool_->PopFreeBlockList(free_block_list_)) {
    // If there are available free slots in the GlobalPool, allocate a free list to the LocalPool with a length of
    // `kFreeBlockNum`.
    ++GetStatistics().allocs_from_tls_free_list;

    block = free_block_list_.head;
    free_block_list_.head = block->next;
    --free_block_list_.length;

    return block;
  }

  // If `free_block_list_` is empty, then get a Block object from the `block_list_` in the current thread and give it to
  // the caller.
  if ((block_list_ != nullptr) && block_list_->idx < kBlockNum) {
    ++GetStatistics().allocs_from_tls_blocks;

    return block_list_->blocks[block_list_->idx++];
  }

  // If the `block_list_` in the current thread is also exhausted, then get a `block_list_` from the `global_pool_`.
  block_list_ = global_pool_->PopBlockList();
  if (TRPC_LIKELY(block_list_)) {
    ++GetStatistics().allocs_from_tls_blocks;

    return block_list_->blocks[block_list_->idx++];
  }

  // If the threshold of the pool is exceeded, the system will allocate a fallback.
  block = AllocateBlock(true);
  if (block) {
    ++GetStatistics().allocs_from_system;
  } else {
    --GetStatistics().total_allocs_num;
  }

  return block;
}

void LocalMemPool::Deallocate(Block* block) noexcept {
  if (block->need_free_to_system == false) {
    // If the length of the free list `free_block_list_` reaches `kFreeBlockNum`, then recycle the free list to the
    // `global_pool`_.
    if (TRPC_UNLIKELY(free_block_list_.length == kFreeBlockNum)) {
      global_pool_->PushFreeBlockList(free_block_list_);
    }

    block->next = free_block_list_.head;
    free_block_list_.head = block;
    ++free_block_list_.length;

    ++GetStatistics().frees_to_tls_free_list;
  } else {
    // Release memory blocks that need to be returned to the system.
    DellocateBlock(block);

    ++GetStatistics().frees_to_system;
  }
  ++GetStatistics().total_frees_num;
}

void LocalMemPool::PrintStatistics() {
  auto tid = std::this_thread::get_id();
  TRPC_FMT_INFO("global mem pool, tid: {} total_allocs_num: {} ", tid, stat_.total_allocs_num);
  TRPC_FMT_INFO("global mem pool, tid: {} total_frees_num: {} ", tid, stat_.total_frees_num);
  TRPC_FMT_INFO("global mem pool, tid: {} allocs_from_tls_free_list: {} ", tid, stat_.allocs_from_tls_free_list);
  TRPC_FMT_INFO("global mem pool, tid: {} allocs_from_tls_blocks: {} ", tid, stat_.allocs_from_tls_blocks);
  TRPC_FMT_INFO("global mem pool, tid: {} frees_to_tls_free_list: {} ", tid, stat_.frees_to_tls_free_list);
  TRPC_FMT_INFO("global mem pool, tid: {} allocs_from_system: {} ", tid, stat_.allocs_from_system);
  TRPC_FMT_INFO("global mem pool, tid: {} frees_to_system: {} ", tid, stat_.frees_to_system);
}

LocalMemPool* GetLocalPoolSlow() noexcept {
  // The `global_pool` will not be released and will end with the process (the order of static destructors for multiple
  // compilation units is not guaranteed).
  // NeverDestroyed is used to prevent asan from reporting memory leaks.
  static trpc::internal::NeverDestroyed<std::unique_ptr<GlobalMemPool[]>> global_pool{
      std::make_unique<GlobalMemPool[]>(kGlobalPoolNum)};
  static std::atomic<uint32_t> pool_idx{0};
  // When accessing the object pool for the first time, create a LocalMemPool object and use the `Round-Robin` algorithm
  // to allocate a GlobalMemPool to the LocalMemPool.
  thread_local std::unique_ptr<LocalMemPool> local_pool = std::make_unique<LocalMemPool>(
      &global_pool.GetReference()[(pool_idx.fetch_add(1, std::memory_order_seq_cst)) % kGlobalPoolNum]);
  return local_pool.get();
}

inline LocalMemPool* GetLocalPool() noexcept {
  thread_local LocalMemPool* local_pool = GetLocalPoolSlow();
  return local_pool;
}

}  // namespace detail

detail::Block* Allocate() noexcept { return detail::GetLocalPool()->Allocate(); }

void Deallocate(detail::Block* block) noexcept { detail::GetLocalPool()->Deallocate(block); }

const Statistics& GetTlsStatistics() noexcept { return detail::GetLocalPool()->GetStatistics(); }

void PrintTlsStatistics() noexcept { detail::GetLocalPool()->PrintStatistics(); }

int PrewarmMemPool(uint32_t block_num) {
  int prewarm_num = 0;

  std::size_t max_block_num = detail::GetMaxBlockNumByPoolThreshold();

  if (block_num > max_block_num || block_num == 0) {
    // If the value is 0 or greater than the maximum value, use the maximum value for preheating.
    block_num = max_block_num;
  }

  std::vector<detail::Block*> result_infos;
  result_infos.resize(block_num);

  for (size_t i = 0; i < block_num; ++i) {
    result_infos[i] = Allocate();
  }

  for (size_t i = 0; i < block_num; ++i) {
    if (result_infos[i]) {
      Deallocate(result_infos[i]);
      prewarm_num++;
    }
  }

  return prewarm_num;
}

}  // namespace trpc::memory_pool::global
