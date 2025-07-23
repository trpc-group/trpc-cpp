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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. flare
// Copyright (C) 2020 Tencent. All rights reserved.
// flare is licensed under the BSD 3-Clause License.
//

#include "trpc/runtime/threadmodel/fiber/detail/stack_allocator_impl.h"

#include <malloc.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdlib>
#include <thread>

#include "trpc/runtime/threadmodel/fiber/detail/assembly.h"
#include "trpc/util/check.h"
#include "trpc/util/deferred.h"
#include "trpc/util/internal/never_destroyed.h"
#include "trpc/util/likely.h"

namespace trpc::fiber::detail {

static bool fiber_use_mmap = true;
static uint32_t fiber_stack_size = 131072;
static bool fiber_stack_enable_guard_page = true;
static uint32_t max_fiber_num_by_mmap = 30 * 1024;
static bool enable_gdb_debug = false;

const uint32_t kPageSize = getpagesize();

// The following source codes are from flare.
// Copied and modified from
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/stack_allocator.cc

// All stacks (whether system stack or user stack) are registered here. This is
// necessary for our GDB plugin to find all the stacks.
//
// Only _actual_ stack allocation / deallocation needs to touch this. For
// allocations / deallocations covered by our object pool, they're irrelevant
// here.
//
// Registration / deregistration can be slow. But that's okay as it's already
// slow enough to _actually_ creating / destroying stacks. These operations
// incur heavy VMA operations.
struct StackRegistry {
  // Listed as public as they're our "public" interfaces to GDB plugin.
  //
  // Code in this TU should use methods below instead of touching these fields.
  void** stacks = nullptr;  // Leaked on exit. Doesn't matter.
  std::size_t used = 0;
  std::size_t capacity = 0;

  // Register a newly-allocated stack.
  //
  // `ptr` should point to stack bottom (i.e. one byte past the stack region).
  // That's where our fiber control block (GDB plugin need it) resides.
  void RegisterStack(void* ptr) {
    std::scoped_lock _(lock_);  // It's slow, so be it.
    ++used;
    auto slot = UnsafeFindSlotOf(nullptr);
    if (slot) {
      *slot = ptr;
      return;
    }

    UnsafeResizeRegistry();
    *UnsafeFindSlotOf(nullptr) = ptr;  // Must succeed this time.
  }

  // Deregister a going-to-be-freed stack. `ptr` points to stack bottom.
  void DeregisterStack(void* ptr) {
    std::scoped_lock _(lock_);

    trpc::ScopedDeferred __([&] {
      // If `stacks` is too large we should consider shrinking it.
      if (capacity > 1024 && capacity / 2 > used) {
        UnsafeShrinkRegistry();
      }
    });

    --used;
    if (auto p = UnsafeFindSlotOf(ptr)) {
      *p = nullptr;
      return;
    }
    TRPC_UNREACHABLE();
  }

 private:
  void** UnsafeFindSlotOf(void* ptr) {
    for (std::size_t i = 0; i != capacity; ++i) {
      if (stacks[i] == ptr) {
        return &stacks[i];
      }
    }
    return nullptr;
  }

  void UnsafeShrinkRegistry() {
    auto new_capacity = capacity / 2;
    TRPC_CHECK(new_capacity);
    auto new_stacks = new void*[new_capacity];
    std::size_t copied = 0;

    memset(new_stacks, 0, new_capacity * sizeof(void*));
    for (std::size_t i = 0; i != capacity; ++i) {
      if (stacks[i]) {
        new_stacks[copied++] = stacks[i];
      }
    }

    TRPC_CHECK_EQ(copied, used);
    TRPC_CHECK_LE(copied, new_capacity);
    capacity = new_capacity;
    delete[] std::exchange(stacks, new_stacks);
  }

  void UnsafeResizeRegistry() {
    if (capacity == 0) {  // We haven't been initialized yet.
      capacity = 8;
      stacks = new void*[capacity];
      memset(stacks, 0, sizeof(void*) * capacity);
    } else {
      auto new_capacity = capacity * 2;
      auto new_stacks = new void*[new_capacity];
      memset(new_stacks, 0, new_capacity * sizeof(void*));
      memcpy(new_stacks, stacks, capacity * sizeof(void*));
      capacity = new_capacity;
      delete[] std::exchange(stacks, new_stacks);
    }
  }

 private:
  std::mutex lock_;
} stack_registry;  // Using global variable here. This makes looking up this
                   // variable easy in GDB plugin.

// End of source codes that are from flare.

void SetFiberPoolCreateWay(bool use_mmap) {
  fiber_use_mmap = use_mmap;
}

void SetFiberStackSize(uint32_t stack_size) {
  bool ok = (stack_size >= kPageSize) && ((stack_size & (kPageSize - 1)) == 0);
  if (!ok) {
    stack_size = ((stack_size + kPageSize) / kPageSize) * kPageSize;
  }
  fiber_stack_size = stack_size;
}

uint32_t GetFiberStackSize() {
  return fiber_stack_size;
}

void SetFiberStackEnableGuardPage(bool flag) {
  fiber_stack_enable_guard_page = flag;
}

void SetFiberPoolNumByMmap(uint32_t fiber_pool_num_by_mmap) {
  max_fiber_num_by_mmap = fiber_pool_num_by_mmap;
}

void SetEnableGdbDebug(bool flag) {
  enable_gdb_debug = flag;
}

// We always align stack top to 1M boundary. This helps our GDB plugin to find
// fiber stacks.
constexpr auto kStackTopAlignment = 1 * 1024 * 1024;

constexpr auto kOutOfMemoryError =
    "Cannot create guard page below fiber stack. Check `/proc/[pid]/maps` to "
    "see if there are too many memory regions. There's a limit at around 64K "
    "by default. If you reached the limit, try either disabling guard page or "
    "increasing `vm.max_map_count` (suggested).";

void InitializeFiberStackMagic(void* ptr) {
  static const char kMagic[] = "TRPC_FIBER_ENTITY";
  static_assert(sizeof(kMagic) <= 32);
  memcpy(ptr, kMagic, sizeof(kMagic));
}

inline std::size_t GetBias() { return fiber_stack_enable_guard_page ? kPageSize : 0; }

inline std::size_t GetAllocationsize() {
  TRPC_CHECK(fiber_stack_size % kPageSize == 0,
             "Stack size ({}) must be a multiple of page size ({}).", fiber_stack_size,
             kPageSize);

  return fiber_stack_size + GetBias();
}

void* AlignedMmapImp(std::size_t desired_size, std::size_t alignment, int prot, int flags) {
  TRPC_CHECK_EQ(alignment & (alignment - 1), std::size_t(0), "`alignment` must be a power of 2.");

  auto allocating = desired_size + alignment;
  auto ptr = mmap(nullptr, allocating, prot, flags, 0, 0);
  if (ptr == MAP_FAILED) {
    return nullptr;
  }
  auto actual_start = reinterpret_cast<std::uintptr_t>(ptr);
  auto actual_end = actual_start + allocating;
  auto desired_end = actual_end / alignment * alignment;
  TRPC_CHECK_GT(desired_end, desired_size);
  auto desired_start = desired_end - desired_size;
  TRPC_CHECK_LT(desired_start, desired_end);

  int ret = -1;
  if (actual_start != desired_start) {
    TRPC_CHECK_LT(actual_start, desired_start);
    ret = munmap(reinterpret_cast<void*>(actual_start), desired_start - actual_start);
    // munmap fail due to exceeds the threshold, so return directly to avoid the program being aborted
    if (TRPC_UNLIKELY(ret != 0)) {
      return nullptr;
    }
  }

  if (actual_end != desired_end) {
    TRPC_CHECK_GT(actual_end, desired_end);
    ret = munmap(reinterpret_cast<void*>(desired_end), actual_end - desired_end);
    // munmap fail due to exceeds the threshold, so we return directly to avoid the program being aborted
    if (TRPC_UNLIKELY(ret != 0)) {
      return nullptr;
    }
  }
  TRPC_CHECK_EQ(desired_end & (alignment - 1), std::size_t(0));
  TRPC_CHECK_EQ(desired_end - desired_start, desired_size);
  return reinterpret_cast<void*>(desired_start);
}

void* AlignedMmap() {
  auto p = AlignedMmapImp(GetAllocationsize(), kStackTopAlignment, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK);
  // p == nullptr，due to exceeds the protected threshold, so we return directly to avoid the program being aborted
  if (TRPC_UNLIKELY(!p)) {
    TRPC_FMT_INFO_EVERY_SECOND(kOutOfMemoryError);
    return nullptr;
  }
  TRPC_CHECK_EQ(reinterpret_cast<std::uintptr_t>(p) & (kPageSize - 1), std::uintptr_t(0));
  if (fiber_stack_enable_guard_page) {
    // mprotect fail due to exceeds the protected threshold, so we return directly to avoid the program being aborted
    int ret = mprotect(p, kPageSize, PROT_NONE);
    if (TRPC_UNLIKELY(ret != 0)) {
      TRPC_FMT_INFO_EVERY_SECOND(kOutOfMemoryError);
      return nullptr;
    }
  }
  auto stack = reinterpret_cast<char*>(p) + GetBias();
  InitializeFiberStackMagic(stack + fiber_stack_size - kPageSize);

  if (enable_gdb_debug) {
    auto stack_bottom = stack + fiber_stack_size;
    // Register the stack.
    stack_registry.RegisterStack(stack_bottom);
  }

  return reinterpret_cast<void*>(stack);
}

void AlignedMunmap(void* ptr) {
  if (enable_gdb_debug) {
    // Remove the stack from our registry.
    auto stack_bottom = reinterpret_cast<char*>(ptr) + fiber_stack_size;
    stack_registry.DeregisterStack(stack_bottom);
  }

  TRPC_PCHECK(munmap(reinterpret_cast<char*>(ptr) - GetBias(), GetAllocationsize()) == 0);
}

void* AlignedMalloc() {
  void* p = aligned_alloc(kPageSize, GetAllocationsize());
  if (!p) {
    return nullptr;
  }
  auto stack = reinterpret_cast<char*>(p);
  InitializeFiberStackMagic(stack + fiber_stack_size - kPageSize);

  if (enable_gdb_debug) {
    auto stack_bottom = stack + fiber_stack_size;
    // Register the stack.
    stack_registry.RegisterStack(stack_bottom);
  }

  return reinterpret_cast<void*>(stack);
}

void AlignedFree(void* aligned_mem) {
  if (enable_gdb_debug) {
    // Remove the stack from our registry.
    auto stack_bottom = reinterpret_cast<char*>(aligned_mem) + fiber_stack_size;
    stack_registry.DeregisterStack(stack_bottom);
  }

  return free(aligned_mem);
}

void* AllocateFiberStack(bool use_mmap) {
  if (TRPC_LIKELY(use_mmap)) {
    return AlignedMmap();
  }

  return AlignedMalloc();
}

void DeallocateFiberStack(void* stack_ptr, bool use_mmap) {
  if (TRPC_LIKELY(use_mmap)) {
    AlignedMunmap(stack_ptr);
  } else {
    AlignedFree(stack_ptr);
  }
}

constexpr std::size_t kFiberStackNum = 32;       // The number of fiber stacks that a Block can accommodate
constexpr std::size_t kBlockNum = 32;            // The number of Blocks contained in a BlockChunk
constexpr std::size_t kFreeFiberStackNum = 32;   // The length of the recycle list for free Blocks
constexpr std::size_t kGlobalPoolNum = 1;        // The number of Global Pools

struct alignas(64) FiberStack {
  // pointer to the next FiberStack
  FiberStack* next = nullptr;
};

struct alignas(64) Block {
  FiberStack* fiber_stacks[kFiberStackNum] = {nullptr};
  size_t idx = 0;
};

struct alignas(64) BlockChunk {
  Block blocks[kBlockNum];
  size_t idx = 0;
  size_t alloc_idx = 0;
};

// The recycle list of fiber stacks, for convenient reuse in the future
struct FreekFiberStacks {
  FiberStack* head = nullptr;
  size_t length = 0;
};

// global resource pool of fiber stack, can have multiple, with a default quantity of 1
class alignas(64) GlobalPool {
 public:
  struct FiberStackManager {
    size_t free_num = 0;
    std::vector<FreekFiberStacks> free_fiber_stack_vec;
  };

  struct BlockManager {
    size_t available_index = 0;
    std::vector<BlockChunk> block_chunks;
  };

 public:
  GlobalPool() noexcept {
    size_t block_chunks_size = max_fiber_num_by_mmap / (kBlockNum * kFiberStackNum) + 1;
    block_manager_.block_chunks.resize(block_chunks_size * 2);
    block_manager_.available_index = 0;

    size_t free_fiber_stack_vec_size = max_fiber_num_by_mmap / kFreeFiberStackNum + 1;
    free_fiber_stack_manager_.free_fiber_stack_vec.resize(free_fiber_stack_vec_size * 2);
    free_fiber_stack_manager_.free_num = 0;

    if (!NewBlockChunk()) {
      TRPC_LOG_ERROR("GlobalPool NewBlockChunk failed.");
      exit(-1);
    }
  }

  ~GlobalPool() {
    for (uint32_t i = 0; i < block_manager_.available_index; ++i) {
      for (uint32_t j = 0; j < block_manager_.block_chunks[i].alloc_idx; ++j) {
        for (uint32_t k = 0; k < kFiberStackNum; ++k) {
          void* stack_ptr = static_cast<void*>(block_manager_.block_chunks[i].blocks[j].fiber_stacks[k]);
          if (stack_ptr != nullptr) {
            DeallocateFiberStack(stack_ptr, true);
          }
        }
      }
    }
  }

  // recycle free fiber stack
  void PushFreeFiberStacks(FreekFiberStacks& free_fiber_stacks) noexcept;

  // pop free fiber stack
  bool PopFreeFiberStacks(FreekFiberStacks& free_fiber_stacks) noexcept;

  // get a Block
  Block* PopBlock() noexcept;

 private:
  // batch request for Blocks from the system
  bool NewBlockChunk() noexcept;

  bool NewBlock() noexcept;

 private:
  size_t alloc_fiber_stack_num_ = 0;
  BlockManager block_manager_;
  std::mutex block_mutex_;

  FiberStackManager free_fiber_stack_manager_;
  std::mutex free_fiber_stack_mutex_;
};

// recycle free fiber stack to Global Pool
void GlobalPool::PushFreeFiberStacks(FreekFiberStacks& free_fiber_stacks) noexcept {
  {
    std::unique_lock lock(free_fiber_stack_mutex_);
    // record the head pointer of the free list in the free_manager
    free_fiber_stack_manager_.free_fiber_stack_vec[free_fiber_stack_manager_.free_num++] = free_fiber_stacks;
  }
  // reset the free list in the Local Pool
  free_fiber_stacks.head = nullptr;
  free_fiber_stacks.length = 0;
}

// get free fiber stack from Global Pool
bool GlobalPool::PopFreeFiberStacks(FreekFiberStacks& free_fiber_stacks) noexcept {
  std::unique_lock lock(free_fiber_stack_mutex_);
  if (free_fiber_stack_manager_.free_num > 0) {
    free_fiber_stacks = free_fiber_stack_manager_.free_fiber_stack_vec[--free_fiber_stack_manager_.free_num];
    lock.unlock();
    return true;
  }

  return false;
}

bool GlobalPool::NewBlockChunk() noexcept {
  BlockChunk* new_block_chunk = &(block_manager_.block_chunks[block_manager_.available_index]);

  new_block_chunk->idx = 0;
  for (uint32_t i = 0; i < kBlockNum; ++i) {
    new_block_chunk->blocks[i].idx = 0;
    for (uint32_t k = 0; k < kFiberStackNum; ++k) {
      void* stack_ptr = AllocateFiberStack(true);
      TRPC_ASSERT(stack_ptr != nullptr);
      new_block_chunk->blocks[i].fiber_stacks[k] = static_cast<FiberStack*>(stack_ptr);
    }
  }

  new_block_chunk->alloc_idx = kBlockNum;

  ++(block_manager_.available_index);

  alloc_fiber_stack_num_ += kBlockNum * kFiberStackNum;

  return true;
}

bool GlobalPool::NewBlock() noexcept {
  BlockChunk* new_block_chunk = &(block_manager_.block_chunks[block_manager_.available_index]);

  new_block_chunk->blocks[new_block_chunk->alloc_idx].idx = 0;
  for (uint32_t k = 0; k < kFiberStackNum; ++k) {
    void* stack_ptr = AllocateFiberStack(true);
    TRPC_ASSERT(stack_ptr != nullptr);
    new_block_chunk->blocks[new_block_chunk->alloc_idx].fiber_stacks[k] =
        static_cast<FiberStack*>(stack_ptr);
  }

  ++(new_block_chunk->alloc_idx);

  alloc_fiber_stack_num_ += kFiberStackNum;

  return true;
}

Block* GlobalPool::PopBlock() noexcept {
  // Check the current BlockChunk to see if there are any free Blocks
  std::unique_lock lock(block_mutex_);
  // block_manager_.block_chunks_ is not necessarily empty
  BlockChunk* block_chunk = &(block_manager_.block_chunks[block_manager_.available_index - 1]);

  if (block_chunk->idx < block_chunk->alloc_idx) {
    // block_chunk->idx_ corresponds to the minimum index of the Blocks that can be allocated in the BlockChunk
    auto res_idx = block_chunk->idx++;
    lock.unlock();
    return &block_chunk->blocks[res_idx];
  }

  if (alloc_fiber_stack_num_ >= max_fiber_num_by_mmap) {
    TRPC_FMT_INFO_EVERY_SECOND("Block Allocate {}, beyond {} limited.",
                               alloc_fiber_stack_num_,
                               max_fiber_num_by_mmap);
    return nullptr;
  }

  // If the current BlockChunk is exhausted, allocate a new Block
  if (TRPC_LIKELY(NewBlock())) {
    // Get the last BlockChunk of the block_manager
    block_chunk = &(block_manager_.block_chunks[block_manager_.available_index]);
    // Allocate the first Block in the BlockChunk
    auto res_idx = block_chunk->idx++;
    if (block_chunk->alloc_idx == kBlockNum) {
      ++(block_manager_.available_index);
    }
    lock.unlock();
    return &block_chunk->blocks[res_idx];
  }
  // Allocate fail, return nullptr
  return nullptr;
}

// local resource pool of fiber stack
class alignas(64) LocalPool {
 public:
  explicit LocalPool(GlobalPool* global_pool) noexcept;
  ~LocalPool();
  // Allocate fiber stack
  bool Allocate(void** stack_ptr, bool* is_system) noexcept;
  // Deallocate fiber stack
  void Deallocate(void* stack_ptr, bool is_system) noexcept;

  // Used to get and set statistics related to fiber stack allocation and deallocation
  inline Statistics& GetStatistics() {
    return stat_;
  }

  // Print statistics related to fiber stack allocation and deallocation
  void PrintStatistics();

 private:
  GlobalPool* global_pool_;

  Block* block_ = nullptr;

  FreekFiberStacks free_fiber_stacks_;

  Statistics stat_;
};

LocalPool::LocalPool(GlobalPool* global_pool) noexcept : global_pool_(global_pool) {
  // When initializing the Local Pool, first request a Block from the Global Pool
  block_ = global_pool->PopBlock();
}

LocalPool::~LocalPool() {
  if (block_ != nullptr) {
    while (block_->idx < kFiberStackNum) {
      if (TRPC_UNLIKELY(free_fiber_stacks_.length == kFreeFiberStackNum)) {
        global_pool_->PushFreeFiberStacks(free_fiber_stacks_);
      }
      auto* fiber_stack = block_->fiber_stacks[block_->idx++];
      fiber_stack->next = free_fiber_stacks_.head;
      free_fiber_stacks_.head = fiber_stack;
      ++free_fiber_stacks_.length;
    }
  }

  if (free_fiber_stacks_.length > 0) {
    global_pool_->PushFreeFiberStacks(free_fiber_stacks_);
  }
}

bool LocalPool::Allocate(void** stack_ptr, bool* is_system) noexcept {
  // If there is still available space in the free slots, allocate an object directly from the Local Pool's free list
  if (TRPC_LIKELY(free_fiber_stacks_.head != nullptr)) {
    ++GetStatistics().total_allocs_num;
    ++GetStatistics().allocs_from_tls_free_list;

    FiberStack* res = free_fiber_stacks_.head;
    free_fiber_stacks_.head = res->next;
    --free_fiber_stacks_.length;

    *stack_ptr = static_cast<void*>(res);
    *is_system = false;

    return true;
  } else if (global_pool_->PopFreeFiberStacks(free_fiber_stacks_)) {
    // If there are available free slots in the Global Pool, allocate a free list of length kFreeFiberStackNum to the
    // Local Pool
    ++GetStatistics().total_allocs_num;
    ++GetStatistics().pop_freelist_from_global;

    FiberStack* res = free_fiber_stacks_.head;
    free_fiber_stacks_.head = res->next;
    --free_fiber_stacks_.length;

    *stack_ptr = static_cast<void*>(res);
    *is_system = false;

    return true;
  } else if ((block_ != nullptr) && block_->idx < kFiberStackNum) {
    // If the Local Pool's Block still has available space, allocate one from the Local Pool's Block
    ++GetStatistics().total_allocs_num;
    ++GetStatistics().allocs_from_tls_block;

    FiberStack* res = block_->fiber_stacks[block_->idx++];
    *stack_ptr = static_cast<void*>(res);
    *is_system = false;

    return true;
  }

  // If the Global Pool's BlockManager still has available Blocks, allocate one Block to the Local Pool
  block_ = global_pool_->PopBlock();
  if (TRPC_LIKELY(block_)) {
    ++GetStatistics().total_allocs_num;
    ++GetStatistics().allocs_from_global_block;

    FiberStack* res = block_->fiber_stacks[block_->idx++];
    *stack_ptr = static_cast<void*>(res);
    *is_system = false;

    return true;
  } else {
    // Allocate from system
    void* ptr = AllocateFiberStack(false);
    if (ptr) {
      ++GetStatistics().total_allocs_num;
      ++GetStatistics().allocs_from_system;

      *stack_ptr = ptr;
      *is_system = true;

      return true;
    }
  }

  return false;
}

void LocalPool::Deallocate(void* stack_ptr, bool is_system)  noexcept {
  if (!is_system) {
    // If the length of the free list in the Local Pool reaches kFreeFiberStackNum, recycle the free list to the Global Pool
    if (TRPC_UNLIKELY(free_fiber_stacks_.length == kFreeFiberStackNum)) {
      global_pool_->PushFreeFiberStacks(free_fiber_stacks_);
      ++GetStatistics().push_freelist_to_global;
    }

    auto* stack = static_cast<FiberStack*>(stack_ptr);
    stack->next = free_fiber_stacks_.head;
    free_fiber_stacks_.head = stack;
    ++free_fiber_stacks_.length;

    ++GetStatistics().total_frees_num;
    ++GetStatistics().frees_to_tls_free_list;
  } else {
    // Freeing memory blocks that were directly allocated from the system
    DeallocateFiberStack(stack_ptr, false);

    ++GetStatistics().total_frees_num;
    ++GetStatistics().frees_to_system;
  }
}

void LocalPool::PrintStatistics() {
  auto tid = std::this_thread::get_id();
  TRPC_LOG_INFO("tid: " << tid << " total_allocs_num: " << stat_.total_allocs_num);
  TRPC_LOG_INFO("tid: " << tid << " allocs_from_tls_free_list: " << stat_.allocs_from_tls_free_list);
  TRPC_LOG_INFO("tid: " << tid << " pop_freelist_from_global: " << stat_.pop_freelist_from_global);
  TRPC_LOG_INFO("tid: " << tid << " allocs_from_tls_block: " << stat_.allocs_from_tls_block);
  TRPC_LOG_INFO("tid: " << tid << " allocs_from_global_block: " << stat_.allocs_from_global_block);
  TRPC_LOG_INFO("tid: " << tid << " allocs_from_system: " << stat_.allocs_from_system);
  TRPC_LOG_INFO("tid: " << tid << " total_frees_num: " << stat_.total_frees_num);
  TRPC_LOG_INFO("tid: " << tid << " frees_to_tls_free_list: " << stat_.frees_to_tls_free_list);
  TRPC_LOG_INFO("tid: " << tid << " push_freelist_to_global: " << stat_.push_freelist_to_global);
  TRPC_LOG_INFO("tid: " << tid << " frees_to_system: " << stat_.frees_to_system);
}

LocalPool* GetLocalPoolSlow() noexcept {
  // global_pool won't be released and will end with the process (the order of static destructors in multiple
  // compilation units is uncertain). NeverDestroyed is used to prevent asan from reporting leaks
  static trpc::internal::NeverDestroyed<std::unique_ptr<GlobalPool[]>> global_pool{
      std::make_unique<GlobalPool[]>(kGlobalPoolNum)};
  static std::atomic<uint32_t> pool_idx = 0;
  // When accessing the object pool for the first time, create a Local Pool and allocate a Global Pool to the Local Pool
  // using the Round-Robin algorithm
  thread_local std::unique_ptr<LocalPool> local_pool = std::make_unique<LocalPool>(
      &global_pool.GetReference()[(pool_idx.fetch_add(1, std::memory_order_seq_cst)) % kGlobalPoolNum]);
  return local_pool.get();
}

inline LocalPool* GetLocalPool() noexcept {
  thread_local LocalPool* local_pool = GetLocalPoolSlow();
  return local_pool;
}

struct FiberPrewarmResutInfo {
  bool is_succ = false;
  void* fiber_stack_ptr = nullptr;
  bool is_system = true;
};

int PrewarmFiberPool(uint32_t fiber_num) {
  int prewarm_num = 0;

  if (fiber_num > max_fiber_num_by_mmap) {
    fiber_num = max_fiber_num_by_mmap;
  }

  std::vector<FiberPrewarmResutInfo> result_infos;
  result_infos.resize(fiber_num);

  for (size_t i = 0; i < fiber_num; ++i) {
    FiberPrewarmResutInfo result_info;

    result_info.is_succ = Allocate(&(result_info.fiber_stack_ptr), &(result_info.is_system));

    result_infos[i] = result_info;
  }

  for (size_t i = 0; i < fiber_num; ++i) {
    FiberPrewarmResutInfo& result_info = result_infos[i];
    if (result_info.is_succ) {
      Deallocate(result_info.fiber_stack_ptr, result_info.is_system);
      prewarm_num++;
    }
  }

  return prewarm_num;
}

bool Allocate(void** stack_ptr, bool* is_system) noexcept {
  return GetLocalPool()->Allocate(stack_ptr, is_system);
}

void Deallocate(void* stack_ptr, bool is_system) noexcept {
  GetLocalPool()->Deallocate(stack_ptr, is_system);
}

void ReuseStackInSanitizer(void* stack_ptr) {
#ifdef TRPC_INTERNAL_USE_ASAN
  trpc::internal::asan::UnpoisonMemoryRegion(stack_ptr, fiber_stack_size);
#endif
}

Statistics& GetTlsStatistics() {
  return GetLocalPool()->GetStatistics();
}

void PrintTlsStatistics() {
  GetLocalPool()->PrintStatistics();
}

}  // namespace trpc::fiber::detail
