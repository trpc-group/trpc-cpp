// Copyright (c) 2024, Tencent Inc.
// All rights reserved.

#pragma once

#include <string>
#include <unordered_map>

#include "trpc/log/trpc_log.h"

namespace trpc::overload_control {

/// @brief Factory of overload control strategy(as template T).
template <class T>
class OverloadControlFactory {
 public:
  /// @brief Singleton
  static OverloadControlFactory* GetInstance() {
    static OverloadControlFactory instance;
    return &instance;
  }

  /// @brief Can't construct by user.
  OverloadControlFactory(const OverloadControlFactory&) = delete;
  OverloadControlFactory& operator=(const OverloadControlFactory&) = delete;

  /// @brief Register the overload control strategy.
  /// @param obj strategy
  /// @note Non-thread-safe
  bool Register(const T& obj);

  /// @brief Get the overload control strategy by name
  /// @param name name of strategy
  /// @return strategy
  T Get(const std::string& name);

  /// @brief Get number of strategies.
  /// @return number of strategies
  /// @note Non-thread-safe
  size_t Size() const { return objs_map_.size(); }

  /// @brief Stop all of overload control strategies.
  //         Mainly used to stop inner thread createdy by each strategy
  /// @note Non-thread-safe.
  void Stop();

  /// @brief Destroy resource of overload control strategies.
  /// @note Non-thread-safe.
  void Destroy();

  /// @brief Clear overload control strategies in this factory.
  /// @note Non-thread-safe.
  void Clear() { objs_map_.clear(); }

 private:
  OverloadControlFactory() = default;

 private:
  // strategies mapping(name->stratege obj)
  std::unordered_map<std::string, T> objs_map_;
};

template <class T>
bool OverloadControlFactory<T>::Register(const T& obj) {
  if (!obj) {
    TRPC_FMT_ERROR("register object is nullptr");
    return false;
  }
  if (Get(obj->Name())) {
    return false;
  }
  if (obj->Init()) {
    objs_map_.emplace(obj->Name(), obj);
    return true;
  }
  TRPC_FMT_ERROR("{} is `Init` failed ", obj->Name());
  return false;
}

template <class T>
T OverloadControlFactory<T>::Get(const std::string& name) {
  T obj = nullptr;
  auto iter = objs_map_.find(name);
  if (iter != objs_map_.end()) {
    obj = iter->second;
  }
  return obj;
}

template <class T>
void OverloadControlFactory<T>::Stop() {
  for (auto& obj : objs_map_) {
    obj.second->Stop();
  }
}

template <class T>
void OverloadControlFactory<T>::Destroy() {
  for (auto& obj : objs_map_) {
    obj.second->Destroy();
  }
  Clear();
}

}  // namespace trpc::overload_control
