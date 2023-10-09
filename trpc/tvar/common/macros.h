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

#include <new>

namespace trpc::tvar {

/// @brief For release array resource in destructor.
/// @private
template <typename T>
struct ArrayDeleter {
  ArrayDeleter() : arr(0) {}

  ~ArrayDeleter() { delete[] arr; }

  T* arr;
};

}  // namespace trpc::tvar

// Array with size bigger than max size allocated in heap using new.
#define TRPC_DEFINE_SMALL_ARRAY(Tp, name, size, maxsize)                                 \
  Tp* name = 0;                                                                          \
  const unsigned name##_size = (size);                                                   \
  const unsigned name##_stack_array_size = (name##_size <= (maxsize) ? name##_size : 0); \
  Tp name##_stack_array[name##_stack_array_size];                                        \
  trpc::tvar::ArrayDeleter<Tp> name##_array_deleter;                                     \
  if (name##_stack_array_size) {                                                         \
    name = name##_stack_array;                                                           \
  } else {                                                                               \
    name = new (::std::nothrow) Tp[name##_size];                                         \
    name##_array_deleter.arr = name;                                                     \
  }
