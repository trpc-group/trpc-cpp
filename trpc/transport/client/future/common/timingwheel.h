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

#include "trpc/util/function.h"
#include "trpc/util/likely.h"
#include "trpc/util/time.h"

namespace trpc::internal {

/// @brief Type of link element of node in double-linked list.
struct TimerLinkEntry {
  TimerLinkEntry* prev{nullptr};
  TimerLinkEntry* next{nullptr};
};

/// @brief Type of node in double-linked list.
template <class T>
struct TimerNode {
  TimerLinkEntry link;
  // When to expire the node, unit/millisecond.
  size_t expire_time_ms{0};
  // Iterator to ease deleting request from hashmap.
  typename T::iterator data_iter;
};

template <class T>
class TimingWheel {
 public:
  constexpr static int kFirstLevelBits = 10;
  constexpr static int kSecondLevelBits = 6;
  constexpr static int kThirdLevelBits = 6;
  constexpr static int kFourLevelBits = 6;
  constexpr static int kFiveLevelBits = 4;

  constexpr static int kFirstLevelSize = 1 << kFirstLevelBits;
  constexpr static int kSecondLevelSize = 1 << kSecondLevelBits;
  constexpr static int kThirdLevelSize = 1 << kThirdLevelBits;
  constexpr static int kFourLevelSize = 1 << kFourLevelBits;
  constexpr static int kFiveLevelSize = 1 << kFiveLevelBits;

  constexpr static int kFirstLevelMask = kFirstLevelSize - 1;
  constexpr static int kSecondLevelMask = kSecondLevelSize - 1;
  constexpr static int kThirdLevelMask = kThirdLevelSize - 1;
  constexpr static int kFourLevelMask = kFourLevelSize - 1;
  constexpr static int kFiveLevelMask = kFiveLevelSize - 1;

  /// @brief Default constructor.
  TimingWheel();

  /// @brief Destructor.
  ~TimingWheel();

  /// @brief Add timer node of current request into timing wheel, for further timeout handling.
  /// @param expire_time_ms When does current request timeout, in millisecond.
  /// @param data_iter Iterator of current request in hashmap.
  /// @return Timer node in timing wheel.
  /// @note Caller do not own the returned pointer.
  TimerNode<T>* Add(size_t expire_time_ms, const typename T::iterator& data_iter);

  /// @brief Delete timer node from timing wheel.
  /// @param node Timer node returned by `Add`.
  void Delete(TimerNode<T>* node);

  /// @brief Check request timeout, and do timeout handle.
  /// @param now_ms Current time in millisecond.
  /// @param timeout_handle Callback to handle request timeout.
  /// @return true: at least one request timeout handling, false: no request timeout handling.
  bool DoTimeout(size_t now_ms, const Function<void(const typename T::iterator&)>& timeout_handle);

 private:
  /// @brief Init timer list.
  void InitTimerList(TimerLinkEntry* link, int size);

  /// @brief Add timer node into timing wheel.
  /// @param node Timer node to add.
  void AddTimerNode(TimerNode<T>* node);

  /// @brief Iterate over current timing list, add nodes into another timer list as time elapse.
  uint32_t Cascade(TimerLinkEntry* link, uint32_t index);

  /// @brief Destroy timer list.
  void DestroyTimerList(TimerLinkEntry* link, int size);

  /// @brief Use new link to replace old link, and re init old link to empty list.
  void ReplaceAndInit(TimerLinkEntry *old_link, TimerLinkEntry *new_link);

  /// @brief Insert new node into head of list.
  void InsertHead(TimerLinkEntry* head_node, TimerLinkEntry* new_node);

  /// @brief Insert new node into tail of list.
  void InsertTail(TimerLinkEntry* head_node, TimerLinkEntry* new_node);

  /// @brief Insert new node between two nodes.
  void Insert(TimerLinkEntry* prev_node, TimerLinkEntry* next_node, TimerLinkEntry* new_node);

 private:
  // Last time handle request timeout, in millisecond.
  size_t last_expire_time_ms_{0};

  // First level timing wheel, store request timeout in future 0 - 1023 ms.
  TimerLinkEntry timer_link_level1_[kFirstLevelSize];

  // Second level timing wheel, store request timeout in future 1024 - 1024*64-1 ms.
  TimerLinkEntry timer_link_level2_[kSecondLevelSize];

  // Third level timing wheel, store request timeout in future 1024*64 - 1024*64*64-1 ms.
  TimerLinkEntry timer_link_level3_[kThirdLevelSize];

  // Forth level timing wheel, store request timeout in future 1024*64*64 - 1024*64*64*64-1 ms.
  TimerLinkEntry timer_link_level4_[kFourLevelSize];

  // Fifth level timing wheel, store request timeout in future 1024*64*64*64 - 1024*64*64*64*16-1 ms.
  TimerLinkEntry timer_link_level5_[kFiveLevelSize];
};

/// @brief Init all the five levels of timing wheel in constructor.
template <typename T>
TimingWheel<T>::TimingWheel() {
  last_expire_time_ms_ = trpc::time::GetMilliSeconds();

  InitTimerList(timer_link_level1_, kFirstLevelSize);
  InitTimerList(timer_link_level2_, kSecondLevelSize);
  InitTimerList(timer_link_level3_, kThirdLevelSize);
  InitTimerList(timer_link_level4_, kFourLevelSize);
  InitTimerList(timer_link_level5_, kFiveLevelSize);
}

/// @brief Destroy all the five levels of timing wheel in destructor.
template <typename T>
TimingWheel<T>::~TimingWheel() {
  DestroyTimerList(timer_link_level1_, kFirstLevelSize);
  DestroyTimerList(timer_link_level2_, kSecondLevelSize);
  DestroyTimerList(timer_link_level3_, kThirdLevelSize);
  DestroyTimerList(timer_link_level4_, kFourLevelSize);
  DestroyTimerList(timer_link_level5_, kFiveLevelSize);

  last_expire_time_ms_ = 0;
}

/// @note Timer node allocated in object_pool.
template <typename T>
TimerNode<T>* TimingWheel<T>::Add(size_t expire_time_ms, const typename T::iterator& data_iter) {
  TimerNode<T>* p = trpc::object_pool::New<TimerNode<T>>();

  if (TRPC_LIKELY(p)) {
    p->expire_time_ms = expire_time_ms;
    p->data_iter = data_iter;

    AddTimerNode(p);
  }

  return p;
}

template <typename T>
void TimingWheel<T>::Delete(TimerNode<T>* node) {
  if (TRPC_LIKELY(node)) {
    TimerLinkEntry* p = &node->link;
    p->prev->next = p->next;
    p->next->prev = p->prev;

    trpc::object_pool::Delete(node);
    node = nullptr;
  }
}

template <typename T>
bool TimingWheel<T>::DoTimeout(size_t now_ms, const Function<void(const typename T::iterator&)>& timeout_handle) {
  bool do_timeout = false;
  while (now_ms >= last_expire_time_ms_) {
    uint32_t index = last_expire_time_ms_ & kFirstLevelMask;
    if (!index &&
        (!Cascade(timer_link_level2_,
            (last_expire_time_ms_ >> kFirstLevelBits) & kSecondLevelMask)) &&
        (!Cascade(timer_link_level3_,
            (last_expire_time_ms_ >> (kFirstLevelBits + kSecondLevelBits)) & kThirdLevelMask)) &&
        (!Cascade(timer_link_level4_,
            (last_expire_time_ms_ >> (kFirstLevelBits + kSecondLevelBits
                + kThirdLevelBits)) & kFourLevelMask))) {
      Cascade(timer_link_level5_,
              (last_expire_time_ms_ >> (kFirstLevelBits + kSecondLevelBits +
                                    kThirdLevelBits + kFourLevelBits)) & kFiveLevelMask);
    }

    TimerLinkEntry tmp_link;
    TimerLinkEntry* p_tmp_link = &tmp_link;

    ReplaceAndInit(&(timer_link_level1_[index]), p_tmp_link);

    p_tmp_link = p_tmp_link->next;
    while (p_tmp_link != &tmp_link) {
      do_timeout = true;
      char* p_addr = static_cast<char*>(static_cast<void*>(p_tmp_link)) - offsetof(TimerNode<T>, link);
      TimerNode<T>* node = static_cast<TimerNode<T>*>(static_cast<void*>(p_addr));
      p_tmp_link = p_tmp_link->next;

      timeout_handle(node->data_iter);

      trpc::object_pool::Delete(node);
      node = nullptr;
    }

    last_expire_time_ms_++;
  }

  return do_timeout;
}

template <typename T>
void TimingWheel<T>::InitTimerList(TimerLinkEntry* link, int size) {
  for (int i = 0; i < size; ++i) {
    link[i].prev = &(link[i]);
    link[i].next = &(link[i]);
  }
}

template <typename T>
void TimingWheel<T>::AddTimerNode(TimerNode<T>* node) {
  TimerLinkEntry* head_link{nullptr};
  uint32_t index = 0;

  int due_time = node->expire_time_ms - last_expire_time_ms_;
  if (due_time < 0) {
    head_link = &(timer_link_level1_[(last_expire_time_ms_ & kFirstLevelMask)]);
  } else if (due_time < kFirstLevelSize) {
    index = node->expire_time_ms & kFirstLevelMask;
    head_link = &(timer_link_level1_[index]);
  } else if (due_time < (1 << (kFirstLevelBits + kSecondLevelBits))) {
    index = (node->expire_time_ms >> kFirstLevelBits) & kSecondLevelMask;
    head_link = &(timer_link_level2_[index]);
  } else if (due_time < (1 << (kFirstLevelBits + kSecondLevelBits +
                                 kThirdLevelBits))) {
    index = (node->expire_time_ms >> (kFirstLevelBits + kSecondLevelBits)) & kThirdLevelMask;
    head_link = &(timer_link_level3_[index]);
  } else if (due_time < (1 << (kFirstLevelBits + kSecondLevelBits +
                               kThirdLevelBits + kFourLevelBits))) {
    index = (node->expire_time_ms >> (kFirstLevelBits + kSecondLevelBits + kThirdLevelBits)) &
            kFourLevelMask;
    head_link = &(timer_link_level4_[index]);
  } else {
    index = (node->expire_time_ms >> (kFirstLevelBits + kSecondLevelBits + kThirdLevelBits + kFourLevelBits))
            & kFiveLevelMask;
    head_link = &(timer_link_level5_[index]);
  }

  InsertTail(head_link, &node->link);
}


template <typename T>
uint32_t TimingWheel<T>::Cascade(TimerLinkEntry* link, uint32_t index) {
  TimerLinkEntry tmp_link;

  ReplaceAndInit(&link[index], &tmp_link);

  TimerLinkEntry* p_tmp_link = tmp_link.next;
  while (p_tmp_link != &tmp_link) {
    char* p_addr = static_cast<char*>(static_cast<void*>(p_tmp_link)) - offsetof(TimerNode<T>, link);
    TimerNode<T>* node = static_cast<TimerNode<T>*>(static_cast<void*>(p_addr));
    p_tmp_link = p_tmp_link->next;

    AddTimerNode(node);
  }

  return index;
}

template <typename T>
void TimingWheel<T>::DestroyTimerList(TimerLinkEntry* link, int size) {
  for (int i = 0; i < size; ++i) {
    TimerLinkEntry tmp_list;

    ReplaceAndInit(&link[i], &tmp_list);

    TimerLinkEntry* p = tmp_list.next;
    while (p != &tmp_list) {
      char* p_addr = static_cast<char*>(static_cast<void*>(p)) - offsetof(TimerNode<T>, link);
      TimerNode<T>* node = static_cast<TimerNode<T>*>(static_cast<void*>(p_addr));
      p = p->next;

      trpc::object_pool::Delete(node);
      node = nullptr;
    }
  }
}

template <typename T>
void TimingWheel<T>::ReplaceAndInit(TimerLinkEntry *old_link, TimerLinkEntry *new_link) {
  new_link->next = old_link->next;
  new_link->next->prev = new_link;
  new_link->prev = old_link->prev;
  new_link->prev->next = new_link;

  // Reinit old link to empty.
  old_link->next = old_link;
  old_link->prev = old_link;
}

template <typename T>
void TimingWheel<T>::InsertHead(TimerLinkEntry* head_node, TimerLinkEntry* new_node) {
  Insert(head_node, head_node->next, new_node);
}

template <typename T>
void TimingWheel<T>::InsertTail(TimerLinkEntry* head_node, TimerLinkEntry* new_node) {
  Insert(head_node->prev, head_node, new_node);
}

template <typename T>
void TimingWheel<T>::Insert(TimerLinkEntry* prev_node, TimerLinkEntry* next_node, TimerLinkEntry* new_node) {
  next_node->prev = new_node;
  new_node->next = next_node;
  new_node->prev = prev_node;
  prev_node->next = new_node;
}

}  // namespace trpc::internal
