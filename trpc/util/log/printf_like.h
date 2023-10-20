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

#include <string>

#include "fmt/printf.h"

#include "trpc/util/log/log.h"

/// @brief printf-like log macros for tRPC-Cpp framework log
#define TRPC_PRT_DEFAULT(instance, level, formats, args...)                                                       \
  do {                                                                                                            \
    const auto& __TRPC_PRINTF_LIKE_INSTANCE__ = ::trpc::LogFactory::GetInstance()->Get();                         \
    if (__TRPC_PRINTF_LIKE_INSTANCE__) {                                                                          \
      if (__TRPC_PRINTF_LIKE_INSTANCE__->ShouldLog(level)) {                                                      \
        TRPC_LOG_TRY {                                                                                            \
          std::string __TRPC_PRINTF_LIKE_MSG__ = ::trpc::Log::LogSprintf(formats, ##args);                        \
          __TRPC_PRINTF_LIKE_INSTANCE__->LogIt(instance, level, __FILE__, __LINE__, __FUNCTION__,                 \
                                               __TRPC_PRINTF_LIKE_MSG__);                                         \
        }                                                                                                         \
        TRPC_LOG_CATCH(instance)                                                                                  \
      }                                                                                                           \
    } else {                                                                                                      \
      if (::trpc::Log::ShouldNoLog(instance, level)) {                                                            \
        TRPC_LOG_TRY {                                                                                            \
          std::string __TRPC_PRINTF_LIKE_MSG__ = ::trpc::Log::LogSprintf(formats, ##args);                        \
          ::trpc::Log::NoLog(instance, level, __FILE__, __LINE__, __FUNCTION__,                                   \
                             std::string_view(__TRPC_PRINTF_LIKE_MSG__.data(), __TRPC_PRINTF_LIKE_MSG__.size())); \
        }                                                                                                         \
        TRPC_LOG_CATCH(instance)                                                                                  \
      }                                                                                                           \
    }                                                                                                             \
  } while (0)


/// @brief printf-like log macros
#define TRPC_PRT(instance, level, formats, args...)                                                               \
  do {                                                                                                            \
    const auto& __TRPC_PRINTF_LIKE_INSTANCE__ = ::trpc::LogFactory::GetInstance()->Get();                         \
    if (__TRPC_PRINTF_LIKE_INSTANCE__) {                                                                          \
      if (__TRPC_PRINTF_LIKE_INSTANCE__->ShouldLog(instance, level)) {                                            \
        TRPC_LOG_TRY {                                                                                            \
          std::string __TRPC_PRINTF_LIKE_MSG__ = ::trpc::Log::LogSprintf(formats, ##args);                        \
          __TRPC_PRINTF_LIKE_INSTANCE__->LogIt(instance, level, __FILE__, __LINE__, __FUNCTION__,                 \
                                               __TRPC_PRINTF_LIKE_MSG__);                                         \
        }                                                                                                         \
        TRPC_LOG_CATCH(instance)                                                                                  \
      }                                                                                                           \
    } else {                                                                                                      \
      if (::trpc::Log::ShouldNoLog(instance, level)) {                                                            \
        TRPC_LOG_TRY {                                                                                            \
          std::string __TRPC_PRINTF_LIKE_MSG__ = ::trpc::Log::LogSprintf(formats, ##args);                        \
          ::trpc::Log::NoLog(instance, level, __FILE__, __LINE__, __FUNCTION__,                                   \
                             std::string_view(__TRPC_PRINTF_LIKE_MSG__.data(), __TRPC_PRINTF_LIKE_MSG__.size())); \
        }                                                                                                         \
        TRPC_LOG_CATCH(instance)                                                                                  \
      }                                                                                                           \
    }                                                                                                             \
  } while (0)

#define TRPC_PRT_IF(instance, condition, level, formats, args...)         \
  if (condition) {                                                        \
    TRPC_PRT_DEFAULT(instance, level, formats, ##args);                   \
  }

#define TRPC_PRT_IF_DEFAULT(instance, condition, level, formats, args...) \
  if (condition) {                                                        \
    TRPC_PRT(instance, level, formats, ##args);                           \
  }

#define TRPC_PRT_EX(context, instance, level, formats, args...)                                           \
  do {                                                                                                    \
    const auto& p = ::trpc::LogFactory::GetInstance()->Get();                                             \
    if (p) {                                                                                              \
      if (p->ShouldLog(instance, level)) {                                                                \
        TRPC_LOG_TRY {                                                                                    \
          auto& filter_data = context->GetAllFilterData();                                                \
          std::string trpc_printf_like_msg = fmt::sprintf(formats, ##args);                               \
          p->LogIt(instance, level, __FILE__, __LINE__, __FUNCTION__, trpc_printf_like_msg, filter_data); \
        }                                                                                                 \
        TRPC_LOG_CATCH(instance)                                                                          \
      }                                                                                                   \
    } else {                                                                                              \
      if (::trpc::Log::ShouldNoLog(instance, level)) {                                                    \
        TRPC_LOG_TRY {                                                                                    \
          std::string trpc_printf_like_msg = fmt::sprintf(formats, ##args);                               \
          ::trpc::Log::NoLog(instance, level, __FILE__, __LINE__, __FUNCTION__,                           \
                             std::string_view(trpc_printf_like_msg.data(), trpc_printf_like_msg.size())); \
        }                                                                                                 \
        TRPC_LOG_CATCH(instance)                                                                          \
      }                                                                                                   \
    }                                                                                                     \
  } while (0)

#define TRPC_PRT_IF_EX(context, instance, condition, level, formats, args...) \
  if (condition) {                                                            \
    TRPC_PRT_EX(context, instance, level, formats, ##args);                   \
  }
