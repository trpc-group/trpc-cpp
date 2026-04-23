#pragma once
#include <list>
#include <unordered_map>
#include <mutex>
#include "trpc/util/fiber/fiber_mutex.h"

template<typename Key, typename Value, typename MutexType = std::mutex>
class LRUCache {
 public:
  explicit LRUCache(size_t capacity) : capacity_(capacity) {}

  bool Put(const Key& key, const Value& value) {
    std::lock_guard<MutexType> lock(mutex_);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
      items_.erase(it->second);
      cache_.erase(it);
    }
    items_.emplace_front(key, value);
    cache_[key] = items_.begin();
    if (cache_.size() > capacity_) {
      auto last = items_.end();
      last--;
      cache_.erase(last->first);
      items_.pop_back();
    }
    return true;
  }

  bool Get(const Key& key, Value& value) {
    std::lock_guard<MutexType> lock(mutex_);
    auto it = cache_.find(key);
    if (it == cache_.end()) return false;
    value = it->second->second;
    items_.splice(items_.begin(), items_, it->second);
    return true;
  }

 private:
  size_t capacity_;
  std::list<std::pair<Key, Value>> items_;
  std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator> cache_;
  MutexType mutex_;
};
