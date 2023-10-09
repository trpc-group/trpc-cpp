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

#pragma once

#include <ctime>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "json/json.h"

#include "trpc/util/deferred.h"
#include "trpc/util/function.h"

namespace trpc::tvar {

/// @brief Set whether to abort when duplicate path found.
/// @note Not thread-safe.
void SetWhetherToAbortOnSamePath(bool enable) noexcept;

/// @brief Check if abort on same path.
bool AbortOnSamePath() noexcept;

class TrpcVarGroup {
 public:
  /// @brief To release leave when tvar destructed.
  struct Handle {
    std::string abs_path;
    Deferred deferred;
  };

  /// @brief To exposed tvar obj in path realized by rel_path.
  /// @param rel_path Relative path.
  /// @param value Function used to get current value.
  /// @param series_value Function used to get history value.
  /// @return Handle obj when success, std::nullopt when failed, even abort when duplicate path found.
  std::optional<Handle> Add(std::string_view rel_path, Function<Json::Value()> value,
                            Function<Json::Value()> series_value = Function<Json::Value()>{});

  /// @brief Same as Add, except that this func need extra parent info.
  /// @param rel_path Relative path.
  /// @param parent Parent + rel_path defines a leave.
  /// @param value Function used to get current value.
  /// @param series_value Function used to get history value.
  /// @note Arguments rel_path, value and series_value stand the same as Add.
  static std::optional<Handle> LinkToParent(
      std::string_view rel_path, TrpcVarGroup* parent = TrpcVarGroup::FindOrCreate("/"),
      Function<Json::Value()> value = Function<Json::Value()>{},
      Function<Json::Value()> series_value = Function<Json::Value()>{});

  /// @brief Find or create a node identified by abs_path.
  /// @param abs_path Absolute path, eg. /a/b/c.
  /// @return Pointer to TrpcVarGroup when success, nullptr or abort when failed.
  static TrpcVarGroup* FindOrCreate(std::string_view abs_path);

  /// @brief Get json data related to leave identified by abs_path.
  /// @param abs_path Absolute path.
  /// @param get_series Get history data or not.
  /// @return Json value when success, std::nullopt when failed.
  static std::optional<Json::Value> TryGet(std::string_view abs_path, bool get_series = false);

 private:
  using Getter = Function<std::optional<Json::Value>(std::string_view, bool)>;

  explicit TrpcVarGroup(std::string abs_path);

  /// @brief Get absolute path of current node.
  const std::string& AbsolutePath() const;

  /// @brief Find the deepest node starting from a relative path.
  /// @param rel_path Relative path to start.
  /// @param left Returned node path cutting out relative path.
  /// @return Pointer to TrpcVarGroup of deepest node.
  TrpcVarGroup* FindLowest(std::string_view rel_path, std::string_view* left);

  /// @brief Similar to FindOrCreate.
  TrpcVarGroup* CreateUpto(std::string_view rel_path);

  /// @brief Add a leave.
  /// @param name Name of the leave.
  /// @param value Function used to get current value.
  /// @return Handle obj used to desctructed.
  std::optional<Handle> AddDirect(std::string_view name, Getter value);

  /// @brief Dump the whole tree into json.
  /// @param get_series Get history data or not.
  /// @return Json data.
  Json::Value Dump(bool get_series = false) const;

  /// @brief Get root node.
  static TrpcVarGroup* Root();

 private:
  friend class RootNode;

  std::string abs_path_;
  mutable std::shared_mutex lock_;
  std::unordered_map<std::string, std::unique_ptr<TrpcVarGroup>> nodes_;
  std::unordered_map<std::string, Getter> leaves_;
};

}  // namespace trpc::tvar
