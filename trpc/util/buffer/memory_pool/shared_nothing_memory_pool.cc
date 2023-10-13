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

#include "trpc/util/buffer/memory_pool/shared_nothing_memory_pool.h"

#include <assert.h>
#include <thread>

#include "trpc/util/log/logging.h"

namespace trpc::memory_pool::shared_nothing {

namespace detail {
// The number of Block objects contained in each chunk.
static constexpr uint32_t kBlocksPerChunk = 32;
// The maximum number of CPU cores, which also represents the number of threads.
static constexpr uint32_t kMaxCpus = 1024;
// The target value for recycling the idle linked list each time.
static constexpr uint32_t kFreeListGoalNum = 24;
//  The threshold for bulk recycling.
static constexpr uint32_t kMaxFreeListNum = 18;
// The redundancy size for adjusting the number of BlockChunk objects each time.
static constexpr uint32_t kBlockChunkRedundant = 100;
namespace {

// The logical CPU ID generator.
static std::atomic<uint32_t> s_cpu_id_gen{0};
// The current size of the memory pool.
static std::atomic<uint32_t> s_current_pool_size{0};

}  // namespace

uint32_t GetCpuId() {
  thread_local uint32_t tls_cpu_id = kInvalidCpuId;
  if (tls_cpu_id == kInvalidCpuId) {
    uint32_t cpu_id = s_cpu_id_gen.fetch_add(1, std::memory_order_relaxed);
    TRPC_ASSERT(cpu_id < kMaxCpus);

    tls_cpu_id = cpu_id;
  }

  return tls_cpu_id;
}

bool DeleteToSystem(Block* block) {
  if (TRPC_UNLIKELY(block->need_free_to_system == true)) {
    block->need_free_to_system = false;
    // Freeing memory to the system, without storing it in the pool.
    DeallocateMemFunc del_fun = GetDeallocateMemFunc();
    TRPC_ASSERT(del_fun);
    del_fun(static_cast<void*>(block));

    ++GetTlsStatistics().frees_to_system;
    return true;
  }
  return false;
}

/// @brief The shared-nothing memory pool management class.
struct SharedNothingMemPool {
  // Store pointers to shared-nothing memory pools for all threads, with the maximum number of CPUs indicating the
  // maximum number of allowed threads.
  static SharedNothingMemPoolImp* all_cpus[kMaxCpus];
};

SharedNothingMemPoolImp* SharedNothingMemPool::all_cpus[kMaxCpus];

BlockChunk& BlockChunkManager::GetBlockChunk(uint32_t index) {
  TRPC_ASSERT(index <= block_chunks_.size() && index > 0);
  return block_chunks_[index];
}

void BlockChunkManager::DoResize(uint32_t size) {
  uint32_t block_chunks_size = block_chunks_.size();
  if (size >= block_chunks_size) {
    uint32_t new_size = std::max(block_chunks_size * 2, size + kBlockChunkRedundant);
    block_chunks_.resize(new_size);
  }
}

void BlockChunkManager::PushFront(BlockChunk& block_chunk, uint32_t block_chunk_id) {
  TRPC_ASSERT(block_chunk_id < block_chunks_.size());
  block_chunk.next_id = front_;
  front_ = block_chunk_id;
}

SharedNothingMemPoolImp::SharedNothingMemPoolImp() {
  cpu_id_ = GetCpuId();
  block_chunk_manager_.DoResize(kBlockChunkRedundant);
  // Get the block size.
  block_size_ = GetMemBlockSize();
  // Due to memory alignment, it is required that the allocated memory size each time is at least the memory alignment
  // size of the size of the block.
  TRPC_ASSERT(block_size_ > alignof(Block));
  chunk_mem_size_ = block_size_ * kBlocksPerChunk;
}

SharedNothingMemPoolImp::~SharedNothingMemPoolImp() {
  // Recycle the memory blocks that cross threads.
  DrainCrossCpuFreelist();

  // Recycle the blocks in `free_block_list_` to `block_chunk_manager_`.
  while (free_block_list_.length > 0) {
    auto* block = free_block_list_.head;
    free_block_list_.head = block->next;
    free_block_list_.length--;

    // Recycle the block to the `block_chunk` in `block_chunk_manager_`.
    BlockChunk& block_chunk = block_chunk_manager_.GetBlockChunk(block->chunk_id);
    if (!block_chunk.free_blocks.head) {
      block_chunk_manager_.PushFront(block_chunk, block->chunk_id);
    }
    block->next = block_chunk.free_blocks.head;
    block_chunk.free_blocks.head = block;
    block_chunk.free_blocks.length++;
  }

  // Release objects in `block_chunk_manager_` on a `block_chunk` basis.
  uint32_t free_chunk_count = 0;
  DeallocateMemFunc del_func = GetDeallocateMemFunc();
  TRPC_ASSERT(del_func);
  while (!block_chunk_manager_.Empty()) {
    uint32_t chunk_id = block_chunk_manager_.GetFrontChunkId();
    auto& block_chunk = block_chunk_manager_.Front();
    block_chunk_manager_.PopFront();
    if (block_chunk.free_blocks.length == kBlocksPerChunk) {
      free_chunk_count++;
      del_func(block_chunk.chunk_addr);
      // After releasing memory, the size of the memory pool becomes smaller.
      s_current_pool_size.fetch_sub(chunk_mem_size_, std::memory_order::memory_order_relaxed);
    } else {
      TRPC_FMT_ERROR("Memory leak, chunk_id = {}, chunk_addr = {}", chunk_id, block_chunk.chunk_addr);
    }
  }

  // If the number of `free_chunk_count` is not equal to the total number of allocated `chunk_id_`, print an error
  // message.
  if (free_chunk_count != chunk_id_) {
    TRPC_FMT_ERROR("Memory leak: alloc count = {}, free count = {}", chunk_id_, free_chunk_count);
  }
}

bool SharedNothingMemPoolImp::NewBlockChunk() {
  auto current_pool_size = s_current_pool_size.load(std::memory_order::memory_order_relaxed);
  if (TRPC_UNLIKELY(current_pool_size >= GetMemPoolThreshold())) {
    TRPC_FMT_INFO_EVERY_SECOND("Block Allocate {}, beyond {} limited.", current_pool_size, GetMemPoolThreshold());
    return false;
  }

  AllocateMemFunc allocate_func = GetAllocateMemFunc();
  TRPC_ASSERT(allocate_func);
  auto* chunk_addr = allocate_func(alignof(Block), chunk_mem_size_);
  if (TRPC_UNLIKELY(chunk_addr == nullptr)) {
    // Failed to allocate large memory block.
    TRPC_FMT_WARN("alloc mem size {} failed !!!", chunk_mem_size_);
    return false;
  }

  ++chunk_id_;  // Increment the allocation count by 1.
  block_chunk_manager_.DoResize(chunk_id_);
  block_chunk_manager_.GetBlockChunk(chunk_id_).chunk_addr = chunk_addr;

  char* addr = static_cast<char*>(chunk_addr);
  // Initialize the memory of the chunk.
  for (uint32_t i = 0; i < kBlocksPerChunk; ++i) {
    Block* block = new (addr) Block{.next = free_block_list_.head,
                                    .cpu_id = GetCpuId(),
                                    .chunk_id = chunk_id_,
                                    .ref_count = 1,
                                    .need_free_to_system = false,
                                    .data = addr + sizeof(Block)};

    free_block_list_.head = block;
    addr += block_size_;
  }

  // Increase the allocation count statistics
  ++GetTlsStatistics().block_chunks_alloc_num;
  s_current_pool_size.fetch_add(chunk_mem_size_, std::memory_order::memory_order_relaxed);

  return true;
}

Block* SharedNothingMemPoolImp::Allocate() {
  DrainCrossCpuFreelist();

  if (TRPC_UNLIKELY(!free_block_list_.head)) {
    // First, allocate `kFreeListGoalNum` targets from `block_chunk_manager_` to `free_blocks`.
    while (!block_chunk_manager_.Empty() && free_block_list_.length < kFreeListGoalNum) {
      auto& block_chunk = block_chunk_manager_.Front();
      block_chunk_manager_.PopFront();

      // Add the `free_blocks` obtained from `block_chunk` directly to the `free_block_list_`.
      if (block_chunk.free_blocks.tail) {
        block_chunk.free_blocks.tail->next = free_block_list_.head;
        free_block_list_.head = block_chunk.free_blocks.head;
        free_block_list_.length += block_chunk.free_blocks.length;
        block_chunk.free_blocks.length = 0;
        block_chunk.free_blocks.head = nullptr;
        block_chunk.free_blocks.tail = nullptr;
      }
    }

    // If `block_chunk_manager_` cannot allocate `kFreeListGoalNum` targets, then allocate a `BlockChunk`.
    while (free_block_list_.length < kFreeListGoalNum) {
      if (TRPC_UNLIKELY(NewBlockChunk() == false)) {
        break;
      }
      free_block_list_.length += kBlocksPerChunk;
    }
  }
  ++GetTlsStatistics().total_allocs_num;

  Block* block = nullptr;
  if (TRPC_LIKELY(free_block_list_.head)) {
    // Allocate a Block object from the `free_block_list_`.
    block = free_block_list_.head;
    block->need_free_to_system = false;
    free_block_list_.head = block->next;
    free_block_list_.length--;
    return block;
  }

  // If the allocation of BlockChunk fails or exceeds the limit, allocate a fallback one.
  AllocateMemFunc allocate_func = GetAllocateMemFunc();
  TRPC_ASSERT(allocate_func);
  char* addr = static_cast<char*>(allocate_func(alignof(Block), block_size_));
  if (TRPC_LIKELY(addr)) {
    block = new (addr) Block{.next = nullptr,
                             .cpu_id = kInvalidCpuId,
                             .chunk_id = kInvalidBlockChunkId,
                             .ref_count = 1,
                             .need_free_to_system = true,
                             .data = addr + sizeof(Block)};

    ++GetTlsStatistics().allocs_from_system;
  } else {
    // Memory allocation failed.
    --GetTlsStatistics().total_allocs_num;
  }

  return block;
}

void SharedNothingMemPoolImp::Deallocate(Block* block) {
  // Free the memory and perform statistics.
  ++GetTlsStatistics().total_frees_num;
  if (DeleteToSystem(block)) {  // Check if it is allocated by the system.
    return;
  }
  block->next = free_block_list_.head;
  free_block_list_.head = block;
  free_block_list_.length++;

  // If there are too many elements in the `free_block_list_`, recycle them to `free_blocks`.
  if (free_block_list_.length > kMaxFreeListNum) {
    while (free_block_list_.head && free_block_list_.length > kFreeListGoalNum) {
      auto* block = free_block_list_.head;
      free_block_list_.head = block->next;
      free_block_list_.length--;

      // Recycle the `block` to `block_chunk_manager_`.
      BlockChunk& block_chunk = block_chunk_manager_.GetBlockChunk(block->chunk_id);
      if (!block_chunk.free_blocks.tail) {
        block_chunk_manager_.PushFront(block_chunk, block->chunk_id);
        block_chunk.free_blocks.head = block;
      } else {
        block_chunk.free_blocks.tail->next = block;
      }

      block->next = nullptr;
      block_chunk.free_blocks.tail = block;
      block_chunk.free_blocks.length++;
    }
  }
}

void SharedNothingMemPoolImp::DeleteCrossCpu(Block* block) {
  if (DeleteToSystem(block)) {  // Check if it is allocated by the system.
    return;
  }

  // For cross-thread deallocation, store the `block` in `xcpu_free_list_`.
  auto& list = xcpu_free_list_;
  auto old = list.load(std::memory_order_relaxed);
  do {
    block->next = old;
  } while (!list.compare_exchange_weak(old, block, std::memory_order_release, std::memory_order_relaxed));

  ++GetTlsStatistics().foreign_frees_num;
}

void SharedNothingMemPoolImp::DrainCrossCpuFreelist() {
  if (!xcpu_free_list_.load(std::memory_order_relaxed)) {
    return;
  }

  // Recycle the block that was cross-thread deallocated to the `free_block_list_` for convenient reuse.
  uint64_t free_num = 0;
  Block* block = xcpu_free_list_.exchange(nullptr, std::memory_order_acquire);
  while (block) {
    Block* next = block->next;

    block->next = free_block_list_.head;
    free_block_list_.head = block;
    free_block_list_.length++;

    block = next;

    ++free_num;
  }

  GetTlsStatistics().cross_cpu_frees_num += free_num;
}

SharedNothingMemPoolImp* GetSharedNothingMemPool(uint32_t cpu_id) {
  thread_local std::unique_ptr<SharedNothingMemPoolImp> tls_pool = nullptr;
  if (!tls_pool) {
    tls_pool = std::make_unique<SharedNothingMemPoolImp>();
    SharedNothingMemPool::all_cpus[cpu_id] = tls_pool.get();
  }

  return tls_pool.get();
}

}  // namespace detail

detail::Block* Allocate() {
  uint32_t tls_cpu_id = detail::GetCpuId();
  detail::Block* block = detail::GetSharedNothingMemPool(tls_cpu_id)->Allocate();

  block->cpu_id = tls_cpu_id;
  return block;
}

void Deallocate(detail::Block* block) {
  uint32_t cpu_id = block->cpu_id;
  TRPC_ASSERT(cpu_id != detail::kInvalidCpuId);

  uint32_t tls_cpu_id = detail::GetCpuId();
  if (cpu_id == tls_cpu_id) {
    // Deallocation within the same thread.
    detail::GetSharedNothingMemPool(cpu_id)->Deallocate(block);
  } else {
    // When using this memory pool, it is necessary to ensure that it is only used in the framework thread, as using it
    // in business threads may cause the thread to exit. If the corresponding memory pool still has Block being used by
    // other threads when it is deallocated, it may cause a memory overflow.
    detail::GetSharedNothingMemPool(cpu_id)->DeleteCrossCpu(block);
  }
}

Statistics& GetTlsStatistics() noexcept {
  thread_local Statistics statistics;
  return statistics;
}

void PrintTlsStatistics() noexcept {
  Statistics& stat = GetTlsStatistics();
  auto tid = std::this_thread::get_id();
  TRPC_FMT_INFO("shared nothing mem pool, tid: {} total_allocs_num: {} ", tid, stat.total_allocs_num);
  TRPC_FMT_INFO("shared nothing mem pool, tid: {} total_frees_num: {} ", tid, stat.total_frees_num);
  TRPC_FMT_INFO("shared nothing mem pool, tid: {} allocs_from_system: {} ", tid, stat.allocs_from_system);
  TRPC_FMT_INFO("shared nothing mem pool, tid: {} frees_to_system: {} ", tid, stat.frees_to_system);
  TRPC_FMT_INFO("shared nothing mem pool, tid: {} cross_cpu_frees_num: {} ", tid, stat.cross_cpu_frees_num);
  TRPC_FMT_INFO("shared nothing mem pool, tid: {} foreign_frees_num: {} ", tid, stat.foreign_frees_num);
  TRPC_FMT_INFO("shared nothing mem pool, tid: {} block_chunks_alloc_num: {} ", tid, stat.block_chunks_alloc_num);
}

}  // namespace trpc::memory_pool::shared_nothing
