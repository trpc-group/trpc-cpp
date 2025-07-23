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

#pragma once

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <vector>

#include "trpc/util/buffer/memory_pool/common.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::memory_pool::shared_nothing {

namespace detail {

/// @brief Invalid CPU ID.
static constexpr uint32_t kInvalidCpuId = 0xffffffff;
/// @brief Invalid BlockChunk ID. IDs greater than 0 are considered valid.
static constexpr uint32_t kInvalidBlockChunkId = 0;

/// @brief Memory blocks that store data in a memory pool.
struct alignas(64) Block {
  Block* next{nullptr};            ///< Points to the next block, where each block is an element in a linked list.
  uint32_t cpu_id{kInvalidCpuId};  ///< Represents the logical ID of a thread on a CPU.
  uint32_t chunk_id{kInvalidBlockChunkId};  ///< The ID corresponding to the BlockChunk object
  std::atomic<std::uint32_t> ref_count{1};  ///< Reference counting, used for smart pointer implementations.
  bool need_free_to_system{true};           ///< Whether it needs to be returned to the system after each use.
  char* data{nullptr};                      ///< The  actual address used to store business data.
};

/// @brief Free block list
struct FreeBlockList {
  Block* head{nullptr};  ///< The head of a linked list.
  Block* tail{nullptr};  ///< The tail of a linked list.
  size_t length{0};      ///< The length of a linked list.
};

/// @brief A management class that allocates multiple Blocks at once.
struct alignas(64) BlockChunk {
  void* chunk_addr;           ///< The starting address of a chunk, used to release memory when a thread exits
  uint32_t next_id{0};        ///< The ID of the next BlockChunk.
  FreeBlockList free_blocks;  ///< The linked list of free Blocks.
};

/// @brief This is used to manage BlockChunk objects and recycle objects allocated from a contiguous block of memory
///        back into a BlockChunk to improve memory contiguity.
class BlockChunkManager {
 public:
  BlockChunkManager() = default;
  ~BlockChunkManager() {}

  /// @brief Get the first BlockChunk object currently available.
  /// @return Reference to the BlockChunk.
  inline BlockChunk& Front() { return block_chunks_[front_]; }

  /// @brief Check if the current BlockChunkManager is empty.
  /// @return true: empty; false: not empty.
  inline bool Empty() { return !front_; }

  /// @brief Get the BlockChunk object at the specified index.
  /// @param index Specified index.
  /// @return Reference to the BlockChunk.
  BlockChunk& GetBlockChunk(uint32_t index);

  /// @brief Used for resizing.
  /// @param size The size that has been reset.
  /// @note The size of `block_chunks_` is kept in sync with the number of allocations of BlockChunk.
  void DoResize(uint32_t size);

  /// @brief Reclaiming BlockChunk.
  /// @param block_chunk The BlockChunk objects that need to be reclaimed.
  /// @param block_chunk_id The IDs of the BlockChunk objects that need to be reclaimed.
  void PushFront(BlockChunk& block_chunk, uint32_t block_chunk_id);

  /// @brief Popping the first one.
  inline void PopFront() { front_ = block_chunks_[front_].next_id; }

  /// @brief Getting the value of the current first chunk ID.
  /// @return Chunk ID value.
  inline uint32_t GetFrontChunkId() { return front_; }

 private:
  // Used to reclaim the BlockChunk objects allocated each time (where the first element is a sentinel element).
  std::vector<BlockChunk> block_chunks_;
  // Identifying the index of the first available BlockChunk.
  uint32_t front_{0};
};

/// @brief The internal code implementation of the shared-nothing memory pool was inspired by the ideas of Seastar's
/// shared-nothing architecture and the code implementation of the high-performance object TLS part of WeChat.
class alignas(64) SharedNothingMemPoolImp {
 public:
  SharedNothingMemPoolImp();
  ~SharedNothingMemPoolImp();

  /// @brief Allocate a Block object for storing data.
  /// @return Block pointer
  Block* Allocate();

  /// @brief Release the Block object.
  /// @param block The pointer to the Block object that needs to be released.
  void Deallocate(Block* block);

  /// @brief Free the Block object to the idle list across threads.
  /// @param block The pointer to the Block object that needs to be released.
  void DeleteCrossCpu(Block* block);

 private:
  // Reclaim the elements in the cross-thread idle list `xcpu_free_list_` to the local thread idle list
  // `free_block_list_`.
  void DrainCrossCpuFreelist();

  // Allocating a large chunk of memory.
  bool NewBlockChunk();

 private:
  uint32_t cpu_id_{kInvalidCpuId};         // The logical ID of the thread where this object is located.
  BlockChunkManager block_chunk_manager_;  // The management class for BlockChunk objects.
  std::size_t block_size_{0};              // The size of each block requested.
  uint32_t chunk_id_{0};                   // The ID of the memory chunk.
  uint32_t chunk_mem_size_{0};             // The size of the memory for each chunk.

  alignas(64) std::atomic<Block*> xcpu_free_list_{nullptr};  // Storing Block objects for cross-thread deallocation.
  FreeBlockList free_block_list_;                            // A linked list of free Block objects in the memory pool.
};

/// @brief To obtain the memory pool corresponding to a specific thread.
/// @param cpu_id Thread logical ID.
/// @return SharedNothingMemPoolImp pointer
SharedNothingMemPoolImp* GetSharedNothingPool(uint32_t cpu_id);

}  // namespace detail

/// @brief Data statistics for the creation and release of the SharedNothingMemPoolImp memory pool.
/// @private For internal use purpose only.
struct alignas(64) Statistics {
  size_t total_allocs_num{0};        ///< Total number of Block objects allocated by the current thread.
  size_t total_frees_num{0};         ///< Number of times the tls releases Block objects.
  size_t allocs_from_system{0};      ///< Number of times the tls directly allocates Block objects from the system.
  size_t frees_to_system{0};         ///< Number of times the tls releases Block objects to the system.
  size_t cross_cpu_frees_num{0};     ///< The number of Block objects that are recycled across CPUs.
  size_t foreign_frees_num{0};       ///< The number of Block objects that are released across CPUs.
  size_t block_chunks_alloc_num{0};  ///< The number of times a chunk is allocated for a Block.
};

/// @brief Allocating memory blocks for a Block.
/// @return Block pointer
/// @private For internal use purpose only.
detail::Block* Allocate();

/// @brief Freeing memory for a Block.
/// @param block Block pointer
/// @private For internal use purpose only.
void Deallocate(detail::Block* block);

/// @brief Getting the statistics of the memory pool for the current thread.
/// @return Statistics
/// @private For internal use purpose only.
Statistics& GetTlsStatistics() noexcept;

/// @brief Print memory allocation/release information for the current thread's memory pool.
/// @private For internal use purpose only.
void PrintTlsStatistics() noexcept;

}  // namespace trpc::memory_pool::shared_nothing
