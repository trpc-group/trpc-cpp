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
#include <mutex>

#include "trpc/util/hazptr/hazptr_object.h"
#include "trpc/util/hazptr/hazptr.h"

namespace trpc::concurrency::detail {

///
/// Implementations of lightly high-performance Concurrent Hashmaps that
/// support Get/Insert/Erase/GetOrInsert/InsertOrAssign.
/// not support Rehash/Iterator
///
/// Readers(Get) are always wait-free.
/// Writers(Insert/Erase/GetOrInsert/InsertOrAssign) are sharded, but take a lock that only locks part of hash bucket.
///

template <
    typename KeyType,
    typename ValueType>
struct NodeT : public trpc::HazptrObject<NodeT<KeyType, ValueType>> {
  std::atomic<NodeT<KeyType, ValueType>*> next{nullptr};
  size_t hash_value{0};
  KeyType key;
  ValueType value;
};

template <
    typename KeyType,
    typename ValueType,
    typename KeyEqual = std::equal_to<KeyType>>
class alignas(64) Bucket {
 public:
  using Node = NodeT<KeyType, ValueType>;

  ~Bucket() {
    Clear();
  }

  bool Get(size_t hash_value, const KeyType& k, ValueType& v) {
    Hazptr hazptr;
    Node* node = hazptr.Keep<Node>(&head_);
    while (node != nullptr) {
      if ((node->hash_value == hash_value) && KeyEqual()(node->key, k)) {
        v = node->value;
        return true;
      }

      node = hazptr.Keep<Node>(&node->next);
    }

    return false;
  }

  bool Insert(size_t hash_value, const KeyType& k, const ValueType& v) {
    Node* node = head_.load(std::memory_order_acquire);
    Node* headnode = node;
    while (node != nullptr) {
      if ((node->hash_value == hash_value) && KeyEqual()(node->key, k)) {
        return false;
      }

      node = node->next.load(std::memory_order_acquire);
    }

    Node* cur = new Node();
    cur->hash_value = hash_value;
    cur->key = k;
    cur->value = v;
    cur->next.store(headnode, std::memory_order_release);

    head_.store(cur, std::memory_order_release);

    return true;
  }

  bool Erase(size_t hash_value, const KeyType& k) {
    std::atomic<Node*>* pos = &head_;
    Node* node = pos->load(std::memory_order_acquire);

    while (node != nullptr) {
      if ((node->hash_value == hash_value) && KeyEqual()(node->key, k)) {
        Node* node_next = node->next.load(std::memory_order_acquire);
        pos->store(node_next, std::memory_order_release);

        node->Retire();
        return true;
      }

      pos = &(node->next);
      node = pos->load(std::memory_order_acquire);
    }

    return false;
  }

  bool GetOrInsert(size_t hash_value, const KeyType& k, const ValueType& v, ValueType& exist) {
    Node* node = head_.load(std::memory_order_acquire);
    Node* headnode = node;
    while (node != nullptr) {
      if ((node->hash_value == hash_value) && KeyEqual()(node->key, k)) {
        exist = node->value;
        return true;
      }

      node = node->next.load(std::memory_order_acquire);
    }

    Node* cur = new Node();
    cur->hash_value = hash_value;
    cur->key = k;
    cur->value = v;
    cur->next.store(headnode, std::memory_order_release);

    head_.store(cur, std::memory_order_release);

    return false;
  }

  bool InsertOrAssign(size_t hash_value, const KeyType& k, const ValueType& v) {
    Node* headnode = head_.load(std::memory_order_acquire);
    Node* node = headnode;
    std::atomic<Node*>* pos = &head_;

    while (node != nullptr) {
      if ((node->hash_value == hash_value) && KeyEqual()(node->key, k)) {
        Node* node_next = node->next.load(std::memory_order_acquire);

        Node* cur = new Node();
        cur->hash_value = hash_value;
        cur->key = k;
        cur->value = v;
        cur->next.store(node_next, std::memory_order_release);

        pos->store(cur, std::memory_order_release);

        node->Retire();
        return false;
      }

      pos = &(node->next);
      node = pos->load(std::memory_order_acquire);
    }

    Node* cur = new Node();
    cur->hash_value = hash_value;
    cur->key = k;
    cur->value = v;
    cur->next.store(headnode, std::memory_order_release);

    head_.store(cur, std::memory_order_release);

    return true;
  }

  void Clear() {
    std::atomic<Node*>* pos = &head_;
    Node* node = pos->load(std::memory_order_acquire);

    while (node != nullptr) {
      Node* tmp = node;

      pos = &(node->next);
      node = pos->load(std::memory_order_acquire);

      tmp->Retire();
    }

    head_.store(nullptr, std::memory_order_release);
  }

  void Reclaim() {
    std::atomic<Node*>* pos = &head_;
    Node* node = pos->load(std::memory_order_acquire);

    while (node != nullptr) {
      Node* tmp = node;

      pos = &(node->next);
      node = pos->load(std::memory_order_acquire);

      tmp->Reclaim();
    }

    head_.store(nullptr, std::memory_order_release);
  }

  void GetAllItems(std::unordered_map<KeyType, ValueType>& map) {
    std::atomic<Node*>* pos = &head_;
    Node* node = pos->load(std::memory_order_acquire);

    while (node != nullptr) {
      map.insert({node->key, node->value});

      pos = &(node->next);
      node = pos->load(std::memory_order_acquire);
    }
  }

 private:
  std::atomic<Node*> head_{nullptr};
};

template <
    typename KeyType,
    typename ValueType,
    typename HashFn = std::hash<KeyType>,
    typename KeyEqual = std::equal_to<KeyType>>
class alignas(64) BucketTable {
 public:
  BucketTable(size_t initial_buckets = 1024, size_t mutex_count = 1024) {
    bucket_count_ = RoundUpPowerOf2(initial_buckets);

    buckets_ = std::make_unique<Bucket<KeyType, ValueType, KeyEqual>[]>(bucket_count_);

    mutex_count_ = RoundUpPowerOf2(mutex_count);

    muts_ = std::make_unique<std::mutex[]>(mutex_count_);
  }

  ~BucketTable() {
    Clear();
  }

  /// @brief Get value in hashmap by key
  /// @param [in] k key
  /// @param [out] v value
  /// @return true: found, false: not found
  bool Get(const KeyType& k, ValueType& v) const {
    size_t h = HashFn()(k);
    size_t idx = GetBucketIdx(h);

    return buckets_[idx].Get(h, k, v);
  }

  /// @brief Insert key/value into hashmap
  /// @param [in] k key
  /// @param [in] v value
  /// @return true: success, false: key exist
  bool Insert(const KeyType& k, const ValueType& v) {
    size_t h = HashFn()(k);
    size_t idx = GetBucketIdx(h);
    size_t mut_idx = GetMutexIdx(h);

    std::unique_lock<std::mutex> lock(muts_[mut_idx]);

    if (buckets_[idx].Insert(h, k, v)) {
      item_num_.fetch_add(1, std::memory_order_release);
      return true;
    }

    return false;
  }

  /// @brief erase item by key
  /// @param [in] k key
  /// @return true: success, false: key not exist
  bool Erase(const KeyType& k) {
    size_t h = HashFn()(k);
    size_t idx = GetBucketIdx(h);
    size_t mut_idx = GetMutexIdx(h);

    std::unique_lock<std::mutex> lock(muts_[mut_idx]);

    if (buckets_[idx].Erase(h, k)) {
      item_num_.fetch_sub(1, std::memory_order_release);
      return true;
    }

    return false;
  }

  /// @brief Get the value by key, if key is not existed, insert key/value into hashmap
  /// @param [in] k key
  /// @param [in] v value
  /// @param [out] exist value
  /// @return true: key is existed in hashmap, return the value
  ///         false: key is not existed, insert key/value into hashmap
  bool GetOrInsert(const KeyType& k, const ValueType& v, ValueType& exist) {
    size_t h = HashFn()(k);
    size_t idx = GetBucketIdx(h);
    size_t mut_idx = GetMutexIdx(h);

    std::unique_lock<std::mutex> lock(muts_[mut_idx]);

    if (!buckets_[idx].GetOrInsert(h, k, v, exist)) {
      item_num_.fetch_add(1, std::memory_order_release);
      return false;
    }

    return true;
  }

  /// @brief Insert key/value into hashmap, if key is existed, update value
  /// @param [in] k key
  /// @param [in] v value
  /// @return true: key is not existed in hashmap, insert success
  ///         false: key is existed, update value
  bool InsertOrAssign(const KeyType& k, const ValueType& v) {
    size_t h = HashFn()(k);
    size_t idx = GetBucketIdx(h);
    size_t mut_idx = GetMutexIdx(h);

    std::unique_lock<std::mutex> lock(muts_[mut_idx]);

    if (buckets_[idx].InsertOrAssign(h, k, v)) {
      item_num_.fetch_add(1, std::memory_order_release);
      return true;
    }

    return false;
  }

  /// @brief Get the item count of the hashmap
  size_t Size() { return item_num_.load(std::memory_order_acquire); }

  /// @brief Clear the hashmap item and reclaim its memory by delay
  void Clear() {
    for (size_t i = 0; i < bucket_count_; ++i) {
      std::unique_lock<std::mutex> lock(muts_[i & (mutex_count_ - 1)]);

      buckets_[i].Clear();
    }

    item_num_.store(0, std::memory_order_release);
  }

  /// @brief Clear the hashmap item and reclaim its memory by immediately
  void Reclaim() {
    for (size_t i = 0; i < bucket_count_; ++i) {
      std::unique_lock<std::mutex> lock(muts_[i & (mutex_count_ - 1)]);

      buckets_[i].Reclaim();
    }

    item_num_.store(0, std::memory_order_release);
  }

  /// @brief Get all hashmap item
  void GetAllItems(std::unordered_map<KeyType, ValueType>& map) {
    for (size_t i = 0; i < bucket_count_; ++i) {
      std::unique_lock<std::mutex> lock(muts_[i & (mutex_count_ - 1)]);

      buckets_[i].GetAllItems(map);
    }
  }

 private:
  size_t GetBucketIdx(size_t hash) const {
    return hash & (bucket_count_ - 1);
  }

  size_t GetMutexIdx(size_t hash) const {
    return hash & (mutex_count_ - 1);
  }

  size_t RoundUpPowerOf2(size_t n) {
    bool size_is_power_of_2 = (n >= 2) && ((n & (n - 1)) == 0);
    if (!size_is_power_of_2) {
      uint64_t tmp = 1;
      while (tmp <= n) {
        tmp <<= 1;
      }

      n = tmp;
    }
    return n;
  }

 private:
  size_t bucket_count_{1024};
  size_t mutex_count_{512};
  std::atomic<size_t> item_num_{0};
  std::unique_ptr<Bucket<KeyType, ValueType, KeyEqual>[]> buckets_;
  std::unique_ptr<std::mutex[]> muts_;
};

template <
    typename KeyType,
    typename ValueType,
    typename HashFn = std::hash<KeyType>,
    typename KeyEqual = std::equal_to<KeyType>>
using LightlyConcurrentHashMapImpl = BucketTable<
    KeyType,
    ValueType,
    HashFn,
    KeyEqual>;

}  // namespace trpc::concurrency::detail
