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

#include <sstream>

#include "trpc/common/config/default_value.h"
#include "trpc/util/log/log.h"

/// @brief Outputs log msg to ostringstream.
#define STREAM_APPENDER(msg)          \
  std::ostringstream __TRPC_STREAM__; \
  __TRPC_STREAM__ << msg

/// @brief stream-like log macros
#define TRPC_STREAM(instance, level, context, msg)                                                          \
  do {                                                                                                      \
    const auto& __TRPC_CPP_STREAM_LOGGER_INSTANCE__ = ::trpc::LogFactory::GetInstance()->Get();             \
    if (__TRPC_CPP_STREAM_LOGGER_INSTANCE__) {                                                              \
      if (__TRPC_CPP_STREAM_LOGGER_INSTANCE__->ShouldLog(instance, level)) {                                \
        TRPC_LOG_TRY {                                                                                      \
          STREAM_APPENDER(msg);                                                                             \
          if (context) {                                                                                    \
            __TRPC_CPP_STREAM_LOGGER_INSTANCE__->LogIt(instance, level, __FILE__, __LINE__, __FUNCTION__,   \
                                                       __TRPC_STREAM__.str(), context->GetAllFilterData()); \
          } else {                                                                                          \
            __TRPC_CPP_STREAM_LOGGER_INSTANCE__->LogIt(instance, level, __FILE__, __LINE__, __FUNCTION__,   \
                                                       __TRPC_STREAM__.str());                              \
          }                                                                                                 \
        }                                                                                                   \
        TRPC_LOG_CATCH(instance)                                                                            \
      }                                                                                                     \
    } else {                                                                                                \
      if (::trpc::Log::ShouldNoLog(instance, level)) {                                                      \
        TRPC_LOG_TRY {                                                                                      \
          STREAM_APPENDER(msg);                                                                             \
          ::trpc::Log::NoLog(instance, level, __FILE__, __LINE__, __FUNCTION__, __TRPC_STREAM__.str());     \
        }                                                                                                   \
        TRPC_LOG_CATCH(instance)                                                                            \
      }                                                                                                     \
    }                                                                                                       \
  } while (0)

/// @brief stream-like log macros for tRPC-Cpp framework log
#define TRPC_STREAM_DEFAULT(instance, level, msg)                                                       \
  do {                                                                                                  \
    const auto& __TRPC_CPP_STREAM_LOGGER_INSTANCE__ = ::trpc::LogFactory::GetInstance()->Get();         \
    if (__TRPC_CPP_STREAM_LOGGER_INSTANCE__) {                                                          \
      if (__TRPC_CPP_STREAM_LOGGER_INSTANCE__->ShouldLog(level)) {                                      \
        TRPC_LOG_TRY {                                                                                  \
          STREAM_APPENDER(msg);                                                                         \
          __TRPC_CPP_STREAM_LOGGER_INSTANCE__->LogIt(instance, level, __FILE__, __LINE__, __FUNCTION__, \
                                                     __TRPC_STREAM__.str());                            \
        }                                                                                               \
        TRPC_LOG_CATCH(instance)                                                                        \
      }                                                                                                 \
    } else {                                                                                            \
      if (::trpc::Log::ShouldNoLog(instance, level)) {                                                  \
        TRPC_LOG_TRY {                                                                                  \
          STREAM_APPENDER(msg);                                                                         \
          ::trpc::Log::NoLog(instance, level, __FILE__, __LINE__, __FUNCTION__, __TRPC_STREAM__.str()); \
        }                                                                                               \
        TRPC_LOG_CATCH(instance)                                                                        \
      }                                                                                                 \
    }                                                                                                   \
  } while (0)

/// @brief stream-like log macros for tRPC-Cpp framework
#define TRPC_STREAM_EX_DEFAULT(instance, level, context, msg)                                               \
  do {                                                                                                      \
    const auto& __TRPC_CPP_STREAM_LOGGER_INSTANCE__ = ::trpc::LogFactory::GetInstance()->Get();             \
    if (__TRPC_CPP_STREAM_LOGGER_INSTANCE__) {                                                              \
      if (__TRPC_CPP_STREAM_LOGGER_INSTANCE__->ShouldLog(level)) {                                          \
        TRPC_LOG_TRY {                                                                                      \
          STREAM_APPENDER(msg);                                                                             \
          __TRPC_CPP_STREAM_LOGGER_INSTANCE__->LogIt(instance, level, __FILE__, __LINE__, __FUNCTION__,     \
                                                     __TRPC_STREAM__.str(), (context)->GetAllFilterData()); \
        }                                                                                                   \
        TRPC_LOG_CATCH(instance)                                                                            \
      }                                                                                                     \
    } else {                                                                                                \
      if (::trpc::Log::ShouldNoLog(instance, level)) {                                                      \
        TRPC_LOG_TRY {                                                                                      \
          STREAM_APPENDER(msg);                                                                             \
          ::trpc::Log::NoLog(instance, level, __FILE__, __LINE__, __FUNCTION__, __TRPC_STREAM__.str());     \
        }                                                                                                   \
        TRPC_LOG_CATCH(instance)                                                                            \
      }                                                                                                     \
    }                                                                                                       \
  } while (0)

/// @brief stream-like log macros
#define TRPC_STREAM_EX(instance, level, context, msg)                                                       \
  do {                                                                                                      \
    const auto& __TRPC_CPP_STREAM_LOGGER_INSTANCE__ = ::trpc::LogFactory::GetInstance()->Get();             \
    if (__TRPC_CPP_STREAM_LOGGER_INSTANCE__) {                                                              \
      if (__TRPC_CPP_STREAM_LOGGER_INSTANCE__->ShouldLog(instance, level)) {                                \
        TRPC_LOG_TRY {                                                                                      \
          STREAM_APPENDER(msg);                                                                             \
          __TRPC_CPP_STREAM_LOGGER_INSTANCE__->LogIt(instance, level, __FILE__, __LINE__, __FUNCTION__,     \
                                                     __TRPC_STREAM__.str(), (context)->GetAllFilterData()); \
        }                                                                                                   \
        TRPC_LOG_CATCH(instance)                                                                            \
      }                                                                                                     \
    } else {                                                                                                \
      if (::trpc::Log::ShouldNoLog(instance, level)) {                                                      \
        TRPC_LOG_TRY {                                                                                      \
          STREAM_APPENDER(msg);                                                                             \
          ::trpc::Log::NoLog(instance, level, __FILE__, __LINE__, __FUNCTION__, __TRPC_STREAM__.str());     \
        }                                                                                                   \
        TRPC_LOG_CATCH(instance)                                                                            \
      }                                                                                                     \
    }                                                                                                       \
  } while (0)

#define TRPC_LOG_MSG(level, msg) TRPC_STREAM_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, level, msg)

#define TRPC_LOGGER_MSG(level, instance, msg) TRPC_STREAM(instance, level, msg)

#define TRPC_LOG_MSG_IF(level, condition, msg)                                \
  if (condition) {                                                            \
    TRPC_STREAM_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, level, msg); \
  }

#define TRPC_LOGGER_MSG_IF(level, condition, instance, msg) \
  if (condition) {                                          \
    TRPC_STREAM(instance, level, msg);                      \
  }

/// @brief uses default logger for logging with context
#define TRPC_LOG_MSG_IF_EX(level, context, condition, msg) \
  if (condition) {                                         \
    TRPC_LOG_MSG_EX(level, context, msg);                  \
  }

#define TRPC_LOGGER_MSG_IF_EX(level, context, instance, condition, msg) \
  if (condition) {                                                      \
    TRPC_LOGGER_MSG_EX(level, context, instance, msg);                  \
  }

#define TRPC_LOGGER_MSG_EX(level, context, instance, msg) TRPC_STREAM_EX(instance, level, context, msg)

#define TRPC_LOG_MSG_EX(level, context, msg) \
  TRPC_STREAM_EX_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, level, context, msg)
