//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/demangle.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <string>
#include <typeinfo>
#include <utility>

namespace trpc::internal {

/// @brief The process of converting a C++ ABI identifier to an original C++ source identifier
/// @param s the name of a variable or object's type neeed to demangle
/// @return string. the name after demangling
/// @note during demangling, the compiler converts the C++ ABI identifier back to its original C++ source identifier,
///       making it easier for programmers to understand and debug the code.
std::string Demangle(const char* s);

/// @brief Get the type name of a variable or object
/// @tparam T types whose names can be obtained by `typeid`.
/// @return string. the name after demangling
template <class T>
std::string GetTypeName() {
  return Demangle(typeid(T).name());
}

/// @brief Get the type name of a right-value variable
/// @tparam T types whose names can be obtained by `typeid`.
/// @param o right-value variable that requires obtaining it's name.
/// @return string. the name after demangling
template <class T>
std::string GetTypeName(T&& o) {
  return Demangle(typeid(std::forward<T>(o)).name());
}

}  // namespace trpc::internal
