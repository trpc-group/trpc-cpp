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

#include "trpc/tvar/common/tvar_group.h"

#include <mutex>
#include <utility>
#include <vector>

#include "trpc/common/config/trpc_config.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string/string_helper.h"

using namespace std::literals;

namespace {

// Let users control whether abort on same path.
bool g_abort_on_same_path{true};

// 1. Either path = '/' or path not ended by '/'.
// 2. "//" is not allowed in path.
#define CHECK_PATH(path)                                   \
  TRPC_ASSERT((path).size() <= 1 || (path).back() != '/'); \
  TRPC_ASSERT((path).find("//") == std::string_view::npos);

// In addition to CHECK_PATH, path is empty or not started by '/'.
#define CHECK_RELATIVE_PATH(path) \
  CHECK_PATH(path);               \
  TRPC_ASSERT((path).empty() || (path).front() != '/')

// In addition to CHECK_PATH, path is empty or started by '/'.
#define CHECK_ABSOLUTE_PATH(path) \
  CHECK_PATH(path);               \
  TRPC_ASSERT((path).empty() || (path).front() == '/')

/// @brief Split path into two parts by last '/'.
std::pair<std::string_view, std::string_view> SplitLastPart(std::string_view path) {
  auto pos = path.find_last_of('/');
  if (pos == std::string_view::npos) {
    return std::pair("", path);
  }
  return std::pair(path.substr(0, pos), path.substr(pos + 1));
}

/// @brief Split path into two parts by first '/'.
std::pair<std::string_view, std::string_view> SplitFirstPart(std::string_view path) {
  auto pos = path.find_first_of('/');
  if (pos == std::string_view::npos) {
    return {path, ""};
  } else {
    return {path.substr(0, pos), path.substr(pos + 1)};
  }
}

/// @brief Replace escaped slash by zero.
std::string SubstituteEscapedSlashForZero(std::string_view path) {
  TRPC_ASSERT(path.find('\0') == std::string_view::npos);
  return trpc::Replace(path, "\\/", "\0"s);
}

/// @brief Reversed to SubstituteEscapedSlashForZero.
std::string SubstituteZeroForEscapedSlash(std::string_view path) {
  return trpc::Replace(path, "\0"s, "\\/");
}

/// @brief Replace zero to plain slash.
std::string UnescapeZeroToPlainSlash(std::string_view path) {
  return trpc::Replace(path, "\0"s, "/");
}

/// @brief Join two paths.
std::string JoinPath(std::string_view a, std::string_view b) {
  if (trpc::EndsWith(b, "/")) {
    b.remove_suffix(1);
  }
  if (trpc::EndsWith(a, "/")) {
    a.remove_suffix(1);
  }
  if (b.empty()) {
    return std::string(a);
  }
  return std::string(a) + "/" + std::string(b);
}

}  // namespace

namespace trpc::tvar {

/// @brief Record in internal var.
void SetWhetherToAbortOnSamePath(bool enable) noexcept {
  g_abort_on_same_path = enable;
}

/// @brief Concerned both internal var and yaml conf.
bool AbortOnSamePath() noexcept {
  return g_abort_on_same_path &&
         trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.abort_on_same_path;
}

std::optional<TrpcVarGroup::Handle> TrpcVarGroup::Add(std::string_view rel_path,
                                                      Function<Json::Value()> value,
                                                      Function<Json::Value()> series_value) {
  // "\\/" is stored as '\0' internally.
  auto real_path = SubstituteEscapedSlashForZero(rel_path);
  CHECK_RELATIVE_PATH(real_path);
  auto&& [path, name] = SplitLastPart(real_path);
  auto* current_dir = CreateUpto(path);
  if (!current_dir) {
    TRPC_ASSERT(!AbortOnSamePath());
    return std::nullopt;
  }
  return current_dir->AddDirect(
      name,
      [f = std::move(value), series_f = std::move(series_value), name = std::string(name)](
      std::string_view expected, bool get_series) -> std::optional<Json::Value> {
        Json::Value jsv;
        if (get_series && series_f) {
          jsv = series_f();
        } else {
          jsv = f();
        }
        if (expected.empty()) {
          return jsv;
        }
        auto real_path = SubstituteEscapedSlashForZero(expected);
        Json::Value* ptr = &jsv;
        auto pieces = Split(real_path, '/');
        for (auto&& e : pieces) {
          auto unescaped = UnescapeZeroToPlainSlash({e.data(), e.size()});
          if (!ptr->isObject() || !ptr->isMember(unescaped)) {
            return {};
          }
          ptr = &(*ptr)[unescaped];
        }
        if (ptr->isNull()) {
          return {};
        }
        return *ptr;
      });
}

std::optional<TrpcVarGroup::Handle> TrpcVarGroup::LinkToParent(
    std::string_view rel_path, TrpcVarGroup* parent, Function<Json::Value()> value,
    Function<Json::Value()> series_value) {
  // Argument parent maybe null as created by FindOrCreate.
  if (!parent) {
    TRPC_ASSERT(!AbortOnSamePath());
    return std::nullopt;
  }

  return parent->Add(rel_path, std::move(value), std::move(series_value));
}

TrpcVarGroup* TrpcVarGroup::FindOrCreate(std::string_view abs_path) {
  auto real_path = SubstituteEscapedSlashForZero(abs_path);
  CHECK_ABSOLUTE_PATH(real_path);
  // substr(1) to remove '/'.
  return Root()->CreateUpto(real_path.substr(1));
}

std::optional<Json::Value> TrpcVarGroup::TryGet(std::string_view abs_path, bool get_series) {
  auto real_path = SubstituteEscapedSlashForZero(abs_path);
  TRPC_ASSERT(!real_path.empty());
  CHECK_ABSOLUTE_PATH(real_path);
  // Special case. Logic below cannot handle this.
  if (real_path == "/") {
    return Root()->Dump(get_series);
  }
  std::string_view left_path;
  // substr(1) to remove '/' at the beginning.
  auto rel_path = real_path.substr(1);
  auto parent = Root()->FindLowest(rel_path, &left_path);
  auto&& [name, rest] = SplitFirstPart(left_path);
  if (name.empty()) {
    return parent->Dump(get_series);
  }
  std::string s(name);
  std::shared_lock lk(parent->lock_);
  // Leaf found
  if (auto iter = parent->leaves_.find(s); iter != parent->leaves_.end()) {
    // Before pass the remaining path inside, we have to restore escaped slashes
    // in it as we replaced them with '\0'.
    return iter->second(SubstituteZeroForEscapedSlash(rest), get_series);
  }

  // No leaf found.
  return {};
}

TrpcVarGroup::TrpcVarGroup(std::string abs_path) : abs_path_(std::move(abs_path)) {
  CHECK_ABSOLUTE_PATH(abs_path);
}

/// @note As the realease order of the static object is uncertain when the program exits,
///       and user may create static obj that depends on TrpcVarGroup, root node is allocated
///       in heap, and never released.
TrpcVarGroup* TrpcVarGroup::Root() {
  static TrpcVarGroup* instance = new TrpcVarGroup("/");
  return instance;
}

const std::string& TrpcVarGroup::AbsolutePath() const { return abs_path_; }

TrpcVarGroup* TrpcVarGroup::FindLowest(std::string_view rel_path, std::string_view* left) {
  CHECK_RELATIVE_PATH(rel_path);
  auto current = this;
  while (!rel_path.empty()) {
    std::scoped_lock lk(current->lock_);
    auto&& [name, rest] = SplitFirstPart(rel_path);
    auto iter = current->nodes_.find(std::string(name));
    if (iter == current->nodes_.end()) {
      break;
    } else {
      current = &*iter->second;
    }
    rel_path = rest;
  }
  if (left) {
    *left = rel_path;
  }
  return current;
}

TrpcVarGroup* TrpcVarGroup::CreateUpto(std::string_view rel_path) {
  CHECK_RELATIVE_PATH(rel_path);
  std::string_view left_path;
  auto current = FindLowest(rel_path, &left_path);

  auto pieces = trpc::Split(left_path, '/');

  for (auto&& e : pieces) {
    auto node_name = std::string(e);
    auto absolute_path = JoinPath(current->AbsolutePath(), node_name);
    auto new_node = std::unique_ptr<TrpcVarGroup>(new TrpcVarGroup(absolute_path));
    std::scoped_lock lk(current->lock_);
    if (current->leaves_.find(node_name) != current->leaves_.end()) {
      TRPC_ASSERT(!AbortOnSamePath());
      return nullptr;
    }
    if (current->nodes_.find(node_name) == current->nodes_.end()) {
      current = &*(current->nodes_[node_name] = std::move(new_node));
    } else {
      current = &*(current->nodes_[node_name]);
    }
  }
  TRPC_ASSERT(trpc::EndsWith(current->AbsolutePath(), rel_path));
  return current;
}

std::optional<TrpcVarGroup::Handle> TrpcVarGroup::AddDirect(std::string_view name, Getter value) {
  std::string leave_name(name);
  {
    std::scoped_lock lk(lock_);
    if (leaves_.find(leave_name) != leaves_.end() || nodes_.find(leave_name) != nodes_.end()) {
      TRPC_ASSERT(!AbortOnSamePath());
      return std::nullopt;
    }
    leaves_[leave_name] = std::move(value);
  }

  auto handle = std::make_optional<TrpcVarGroup::Handle>();
  handle->deferred = Deferred([this, leave_name] {
    std::scoped_lock lk(lock_);
    TRPC_ASSERT(leaves_.erase(leave_name) == 1);
  });
  handle->abs_path = JoinPath(AbsolutePath(), leave_name);
  return handle;
}

Json::Value TrpcVarGroup::Dump(bool get_series) const {
  Json::Value jsv;

  std::shared_lock lk(lock_);
  for (auto&& [k, v] : nodes_) {
    jsv[UnescapeZeroToPlainSlash(k)] = v->Dump(get_series);
  }
  for (auto&& [k, v] : leaves_) {
    jsv[UnescapeZeroToPlainSlash(k)] = *v("", get_series);
  }
  return jsv;
}

}  // namespace trpc::tvar
