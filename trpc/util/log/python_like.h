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

#include <string>

#include "fmt/core.h"
#include "fmt/format.h"
#include "fmt/std.h"

#include "trpc/util/log/log.h"

/// @brief python-like style log macros for  tRPC-Cpp framework log
#define TRPC_FMT_DEFAULT(instance, level, formats, args...)                                       \
  do {                                                                                            \
    const auto& __TRPC_PYTHON_LIKE_INSTANCE__ = ::trpc::LogFactory::GetInstance()->Get();         \
    if (__TRPC_PYTHON_LIKE_INSTANCE__) {                                                          \
      if (__TRPC_PYTHON_LIKE_INSTANCE__->ShouldLog(level)) {                                      \
        TRPC_LOG_TRY {                                                                            \
          __TRPC_PYTHON_LIKE_INSTANCE__->LogIt(instance, level, __FILE__, __LINE__, __FUNCTION__, \
                                               ::trpc::Log::LogFormat(formats, ##args));          \
        }                                                                                         \
        TRPC_LOG_CATCH(instance)                                                                  \
      }                                                                                           \
    } else {                                                                                      \
      if (::trpc::Log::ShouldNoLog(instance, level)) {                                            \
        TRPC_LOG_TRY {                                                                            \
          ::trpc::Log::NoLog(instance, level, __FILE__, __LINE__, __FUNCTION__,                   \
                             ::trpc::Log::LogFormat(formats, ##args));                            \
        }                                                                                         \
        TRPC_LOG_CATCH(instance)                                                                  \
      }                                                                                           \
    }                                                                                             \
  } while (0)


/// @brief python-like style log macros
#define TRPC_FMT(instance, level, formats, args...)                                               \
  do {                                                                                            \
    const auto& __TRPC_PYTHON_LIKE_INSTANCE__ = ::trpc::LogFactory::GetInstance()->Get();         \
    if (__TRPC_PYTHON_LIKE_INSTANCE__) {                                                          \
      if (__TRPC_PYTHON_LIKE_INSTANCE__->ShouldLog(instance, level)) {                            \
        TRPC_LOG_TRY {                                                                            \
          __TRPC_PYTHON_LIKE_INSTANCE__->LogIt(instance, level, __FILE__, __LINE__, __FUNCTION__, \
                                               ::trpc::Log::LogFormat(formats, ##args));          \
        }                                                                                         \
        TRPC_LOG_CATCH(instance)                                                                  \
      }                                                                                           \
    } else {                                                                                      \
      if (::trpc::Log::ShouldNoLog(instance, level)) {                                            \
        TRPC_LOG_TRY {                                                                            \
          ::trpc::Log::NoLog(instance, level, __FILE__, __LINE__, __FUNCTION__,                   \
                             ::trpc::Log::LogFormat(formats, ##args));                            \
        }                                                                                         \
        TRPC_LOG_CATCH(instance)                                                                  \
      }                                                                                           \
    }                                                                                             \
  } while (0)

#define TRPC_FMT_IF(instance, condition, level, formats, args...) \
  if (condition) {                                                \
    TRPC_FMT(instance, level, formats, ##args);                   \
  }

#define TRPC_FMT_EX(context, instance, level, formats, args...)                                                   \
  do {                                                                                                            \
    const auto& p = ::trpc::LogFactory::GetInstance()->Get();                                                     \
    if (p) {                                                                                                      \
      if (p->ShouldLog(instance, level)) {                                                                        \
        TRPC_LOG_TRY {                                                                                            \
          auto& filter_data = context->GetAllFilterData();                                                        \
          p->LogIt(instance, level, __FILE__, __LINE__, __FUNCTION__, fmt::format(formats, ##args), filter_data); \
        }                                                                                                         \
        TRPC_LOG_CATCH(instance)                                                                                  \
      }                                                                                                           \
    } else {                                                                                                      \
      if (::trpc::Log::ShouldNoLog(instance, level)) {                                                            \
        TRPC_LOG_TRY {                                                                                            \
          ::trpc::Log::NoLog(instance, level, __FILE__, __LINE__, __FUNCTION__, fmt::format(formats, ##args));    \
        }                                                                                                         \
        TRPC_LOG_CATCH(instance)                                                                                  \
      }                                                                                                           \
    }                                                                                                             \
  } while (0)

#define TRPC_FMT_IF_EX(context, instance, condition, level, formats, args...) \
  if (condition) {                                                            \
    TRPC_FMT_EX(context, instance, level, formats, ##args);                   \
  }
