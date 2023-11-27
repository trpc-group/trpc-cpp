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

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <string>

#include "trpc/common/config/default_value.h"
#include "trpc/util/log/log.h"
#include "trpc/util/log/printf_like.h"
#include "trpc/util/log/python_like.h"
#include "trpc/util/log/stream_like.h"

namespace trpc::log {

/// @brief Initialize the "default" log plugin from the yaml configuration
/// @return true/false means success/failure
bool Init();

/// @brief Destroy the "default" log plugin
void Destroy();

/// @brief Determines if level can log in this instance
/// @param instance_name
/// @param level
/// @return If the log min_level is less than level, it returns true; otherwise, it returns false.
///// For example, if min_level=1 (debug) and info is the level, then true is returned.
bool IsLogOn(const char* instance_name, trpc::Log::Level level);

using trpc::kTrpcLogCacheStringDefault;

}  // namespace trpc::log

/// @brief Set up the log sink instance and output the log message.
/// @param instance  Log instance
/// @param level     Log level
/// @param msg       Log message
#define TRPC_LOG(instance, level, msg)                                                        \
  do {                                                                                        \
    const auto& __TRPC_LOG_INSTANCE__ = ::trpc::LogFactory::GetInstance()->Get();             \
    if (__TRPC_LOG_INSTANCE__) {                                                              \
      if (__TRPC_LOG_INSTANCE__->ShouldLog(instance, level))                                  \
        __TRPC_LOG_INSTANCE__->LogIt(instance, level, __FILE__, __LINE__, __FUNCTION__, msg); \
    } else {                                                                                  \
      ::trpc::Log::NoLog(instance, level, __FILE__, __LINE__, __FUNCTION__, msg);             \
    }                                                                                         \
  } while (0)

#ifndef TRPC_LOG_THROW
#define TRPC_LOG_TRY try
#define TRPC_LOG_CATCH(instance)                                                                                       \
  catch (const std::exception& ex) {                                                                                   \
    ::trpc::Log::NoLog(instance, ::trpc::Log::critical, __FILE__, __LINE__, __FUNCTION__, ex.what(), false);           \
  }                                                                                                                    \
  catch (...) {                                                                                                        \
    ::trpc::Log::NoLog(instance, ::trpc::Log::critical, __FILE__, __LINE__, __FUNCTION__, "Unknown exception", false); \
  }
#else
#define TRPC_LOG_TRY
#define TRPC_LOG_CATCH(instance)
#endif

/// @brief python-like style log macros
#define TRPC_LOGGER_FMT_TRACE(instance, format, args...) TRPC_FMT(instance, ::trpc::Log::trace, format, ##args)
#define TRPC_LOGGER_FMT_DEBUG(instance, format, args...) TRPC_FMT(instance, ::trpc::Log::debug, format, ##args)
#define TRPC_LOGGER_FMT_INFO(instance, format, args...) TRPC_FMT(instance, ::trpc::Log::info, format, ##args)
#define TRPC_LOGGER_FMT_WARN(instance, format, args...) TRPC_FMT(instance, ::trpc::Log::warn, format, ##args)
#define TRPC_LOGGER_FMT_ERROR(instance, format, args...) TRPC_FMT(instance, ::trpc::Log::error, format, ##args)
#define TRPC_LOGGER_FMT_CRITICAL(instance, format, args...) TRPC_FMT(instance, ::trpc::Log::critical, format, ##args)

#define TRPC_LOGGER_FMT_TRACE_IF(instance, condition, format, args...) \
  TRPC_FMT_IF(instance, condition, ::trpc::Log::trace, format, ##args)
#define TRPC_LOGGER_FMT_DEBUG_IF(instance, condition, format, args...) \
  TRPC_FMT_IF(instance, condition, ::trpc::Log::debug, format, ##args)
#define TRPC_LOGGER_FMT_INFO_IF(instance, condition, format, args...) \
  TRPC_FMT_IF(instance, condition, ::trpc::Log::info, format, ##args)
#define TRPC_LOGGER_FMT_WARN_IF(instance, condition, format, args...) \
  TRPC_FMT_IF(instance, condition, ::trpc::Log::warn, format, ##args)
#define TRPC_LOGGER_FMT_ERROR_IF(instance, condition, format, args...) \
  TRPC_FMT_IF(instance, condition, ::trpc::Log::error, format, ##args)
#define TRPC_LOGGER_FMT_CRITICAL_IF(instance, condition, format, args...) \
  TRPC_FMT_IF(instance, condition, ::trpc::Log::critical, format, ##args)

#define TRPC_LOGGER_FMT_TRACE_EX(context, instance, format, args...) \
  TRPC_FMT_EX(context, instance, ::trpc::Log::trace, format, ##args)
#define TRPC_LOGGER_FMT_DEBUG_EX(context, instance, format, args...) \
  TRPC_FMT_EX(context, instance, ::trpc::Log::debug, format, ##args)
#define TRPC_LOGGER_FMT_INFO_EX(context, instance, format, args...) \
  TRPC_FMT_EX(context, instance, ::trpc::Log::info, format, ##args)
#define TRPC_LOGGER_FMT_WARN_EX(context, instance, format, args...) \
  TRPC_FMT_EX(context, instance, ::trpc::Log::warn, format, ##args)
#define TRPC_LOGGER_FMT_ERROR_EX(context, instance, format, args...) \
  TRPC_FMT_EX(context, instance, ::trpc::Log::error, format, ##args)
#define TRPC_LOGGER_FMT_CRITICAL_EX(context, instance, format, args...) \
  TRPC_FMT_EX(context, instance, ::trpc::Log::critical, format, ##args)

#define TRPC_LOGGER_FMT_TRACE_IF_EX(context, instance, condition, format, args...) \
  TRPC_FMT_IF_EX(context, instance, condition, ::trpc::Log::trace, format, ##args)
#define TRPC_LOGGER_FMT_DEBUG_IF_EX(context, instance, condition, format, args...) \
  TRPC_FMT_IF_EX(context, instance, condition, ::trpc::Log::debug, format, ##args)
#define TRPC_LOGGER_FMT_INFO_IF_EX(context, instance, condition, format, args...) \
  TRPC_FMT_IF_EX(context, instance, condition, ::trpc::Log::info, format, ##args)
#define TRPC_LOGGER_FMT_WARN_IF_EX(context, instance, condition, format, args...) \
  TRPC_FMT_IF_EX(context, instance, condition, ::trpc::Log::warn, format, ##args)
#define TRPC_LOGGER_FMT_ERROR_IF_EX(context, instance, condition, format, args...) \
  TRPC_FMT_IF_EX(context, instance, condition, ::trpc::Log::error, format, ##args)
#define TRPC_LOGGER_FMT_CRITICAL_IF_EX(context, instance, condition, format, args...) \
  TRPC_FMT_IF_EX(context, instance, condition, ::trpc::Log::critical, format, ##args)

/// @brief Log will be output to the "default" logger instance provided by the framework "default" plugin.
/// @note There is only one parameter, which indicates the log content to be printed.
#define TRPC_FMT_TRACE(format, args...) TRPC_LOGGER_FMT_TRACE(::trpc::log::kTrpcLogCacheStringDefault, format, ##args)
#define TRPC_FMT_DEBUG(format, args...) TRPC_LOGGER_FMT_DEBUG(::trpc::log::kTrpcLogCacheStringDefault, format, ##args)
#define TRPC_FMT_INFO(format, args...) TRPC_LOGGER_FMT_INFO(::trpc::log::kTrpcLogCacheStringDefault, format, ##args)
#define TRPC_FMT_WARN(format, args...) TRPC_LOGGER_FMT_WARN(::trpc::log::kTrpcLogCacheStringDefault, format, ##args)
#define TRPC_FMT_ERROR(format, args...) TRPC_LOGGER_FMT_ERROR(::trpc::log::kTrpcLogCacheStringDefault, format, ##args)
#define TRPC_FMT_CRITICAL(format, args...) \
  TRPC_LOGGER_FMT_CRITICAL(::trpc::log::kTrpcLogCacheStringDefault, format, ##args)

/// @brief Log macro interface with IF condition.
/// @note Conditional log macros, that is, if the condition is met, then the log is output, otherwise no output.
#define TRPC_FMT_TRACE_IF(condition, format, args...) \
  TRPC_LOGGER_FMT_TRACE_IF(::trpc::log::kTrpcLogCacheStringDefault, condition, format, ##args)
#define TRPC_FMT_DEBUG_IF(condition, format, args...) \
  TRPC_LOGGER_FMT_DEBUG_IF(::trpc::log::kTrpcLogCacheStringDefault, condition, format, ##args)
#define TRPC_FMT_INFO_IF(condition, format, args...) \
  TRPC_LOGGER_FMT_INFO_IF(::trpc::log::kTrpcLogCacheStringDefault, condition, format, ##args)
#define TRPC_FMT_WARN_IF(condition, format, args...) \
  TRPC_LOGGER_FMT_WARN_IF(::trpc::log::kTrpcLogCacheStringDefault, condition, format, ##args)
#define TRPC_FMT_ERROR_IF(condition, format, args...) \
  TRPC_LOGGER_FMT_ERROR_IF(::trpc::log::kTrpcLogCacheStringDefault, condition, format, ##args)
#define TRPC_FMT_CRITICAL_IF(condition, format, args...) \
  TRPC_LOGGER_FMT_CRITICAL_IF(::trpc::log::kTrpcLogCacheStringDefault, condition, format, ##args)

/// @brief printf-like log macros.
#define TRPC_LOGGER_PRT_TRACE(instance, format, args...) TRPC_PRT(instance, ::trpc::Log::trace, format, ##args)
#define TRPC_LOGGER_PRT_DEBUG(instance, format, args...) TRPC_PRT(instance, ::trpc::Log::debug, format, ##args)
#define TRPC_LOGGER_PRT_INFO(instance, format, args...) TRPC_PRT(instance, ::trpc::Log::info, format, ##args)
#define TRPC_LOGGER_PRT_WARN(instance, format, args...) TRPC_PRT(instance, ::trpc::Log::warn, format, ##args)
#define TRPC_LOGGER_PRT_ERROR(instance, format, args...) TRPC_PRT(instance, ::trpc::Log::error, format, ##args)
#define TRPC_LOGGER_PRT_CRITICAL(instance, format, args...) TRPC_PRT(instance, ::trpc::Log::critical, format, ##args)

/// @brief printf-like style log macros with conditions for tRPC-Cpp framework
#define TRPC_LOGGER_PRT_TRACE_IF(instance, condition, format, args...) \
  TRPC_PRT_IF(instance, condition, ::trpc::Log::trace, format, ##args)
#define TRPC_LOGGER_PRT_DEBUG_IF(instance, condition, format, args...) \
  TRPC_PRT_IF(instance, condition, ::trpc::Log::debug, format, ##args)
#define TRPC_LOGGER_PRT_INFO_IF(instance, condition, format, args...) \
  TRPC_PRT_IF(instance, condition, ::trpc::Log::info, format, ##args)
#define TRPC_LOGGER_PRT_WARN_IF(instance, condition, format, args...) \
  TRPC_PRT_IF(instance, condition, ::trpc::Log::warn, format, ##args)
#define TRPC_LOGGER_PRT_ERROR_IF(instance, condition, format, args...) \
  TRPC_PRT_IF(instance, condition, ::trpc::Log::error, format, ##args)
#define TRPC_LOGGER_PRT_CRITICAL_IF(instance, condition, format, args...) \
  TRPC_PRT_IF(instance, condition, ::trpc::Log::critical, format, ##args)

/// @brief printf-like style log macros with conditions
#define TRPC_LOGGER_PRT_TRACE_EX(context, instance, format, args...) \
  TRPC_PRT_EX(context, instance, ::trpc::Log::trace, format, ##args)
#define TRPC_LOGGER_PRT_DEBUG_EX(context, instance, format, args...) \
  TRPC_PRT_EX(context, instance, ::trpc::Log::debug, format, ##args)
#define TRPC_LOGGER_PRT_INFO_EX(context, instance, format, args...) \
  TRPC_PRT_EX(context, instance, ::trpc::Log::info, format, ##args)
#define TRPC_LOGGER_PRT_WARN_EX(context, instance, format, args...) \
  TRPC_PRT_EX(context, instance, ::trpc::Log::warn, format, ##args)
#define TRPC_LOGGER_PRT_ERROR_EX(context, instance, format, args...) \
  TRPC_PRT_EX(context, instance, ::trpc::Log::error, format, ##args)
#define TRPC_LOGGER_PRT_CRITICAL_EX(context, instance, format, args...) \
  TRPC_PRT_EX(context, instance, ::trpc::Log::critical, format, ##args)

/// @brief printf-like style log macros with conditions and context
#define TRPC_LOGGER_PRT_TRACE_IF_EX(context, instance, condition, format, args...) \
  TRPC_PRT_IF_EX(context, instance, condition, ::trpc::Log::trace, format, ##args)
#define TRPC_LOGGER_PRT_DEBUG_IF_EX(context, instance, condition, format, args...) \
  TRPC_PRT_IF_EX(context, instance, condition, ::trpc::Log::debug, format, ##args)
#define TRPC_LOGGER_PRT_INFO_IF_EX(context, instance, condition, format, args...) \
  TRPC_PRT_IF_EX(context, instance, condition, ::trpc::Log::info, format, ##args)
#define TRPC_LOGGER_PRT_WARN_IF_EX(context, instance, condition, format, args...) \
  TRPC_PRT_IF_EX(context, instance, condition, ::trpc::Log::warn, format, ##args)
#define TRPC_LOGGER_PRT_ERROR_IF_EX(context, instance, condition, format, args...) \
  TRPC_PRT_IF_EX(context, instance, condition, ::trpc::Log::error, format, ##args)
#define TRPC_LOGGER_PRT_CRITICAL_IF_EX(context, instance, condition, format, args...) \
  TRPC_PRT_IF_EX(context, instance, condition, ::trpc::Log::critical, format, ##args)

/// @brief Log will be output to the "default" logger instance provided by the framework "default" plugin.
#define TRPC_PRT_TRACE(format, args...) \
  TRPC_PRT_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, ::trpc::Log::trace, format, ##args)
#define TRPC_PRT_DEBUG(format, args...) \
  TRPC_PRT_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, ::trpc::Log::debug, format, ##args)
#define TRPC_PRT_INFO(format, args...) \
  TRPC_PRT_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, ::trpc::Log::info, format, ##args)
#define TRPC_PRT_WARN(format, args...) \
  TRPC_PRT_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, ::trpc::Log::warn, format, ##args)
#define TRPC_PRT_ERROR(format, args...) \
  TRPC_PRT_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, ::trpc::Log::error, format, ##args)
#define TRPC_PRT_CRITICAL(format, args...) \
  TRPC_PRT_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, ::trpc::Log::critical, format, ##args)

/// @brief Log macro for the framework default log with conditions.
#define TRPC_PRT_TRACE_IF(condition, format, args...) \
  TRPC_PRT_IF_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, condition, ::trpc::Log::trace, format, ##args)
#define TRPC_PRT_DEBUG_IF(condition, format, args...) \
  TRPC_PRT_IF_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, condition, ::trpc::Log::debug, format, ##args)
#define TRPC_PRT_INFO_IF(condition, format, args...) \
  TRPC_PRT_IF_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, condition, ::trpc::Log::info, format, ##args)
#define TRPC_PRT_WARN_IF(condition, format, args...) \
  TRPC_PRT_IF_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, condition, ::trpc::Log::warn, format, ##args)
#define TRPC_PRT_ERROR_IF(condition, format, args...) \
  TRPC_PRT_IF_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, condition, ::trpc::Log::error, format, ##args)
#define TRPC_PRT_CRITICAL_IF(condition, format, args...) \
  TRPC_PRT_IF_DEFAULT(::trpc::log::kTrpcLogCacheStringDefault, condition, ::trpc::Log::critical, format, ##args)

/// @brief stream-like log macros for tRPC-Cpp framework
#define TRPC_LOG_TRACE(msg) TRPC_LOG_MSG(::trpc::Log::trace, msg)
#define TRPC_LOG_DEBUG(msg) TRPC_LOG_MSG(::trpc::Log::debug, msg)
#define TRPC_LOG_INFO(msg) TRPC_LOG_MSG(::trpc::Log::info, msg)
#define TRPC_LOG_WARN(msg) TRPC_LOG_MSG(::trpc::Log::warn, msg)
#define TRPC_LOG_ERROR(msg) TRPC_LOG_MSG(::trpc::Log::error, msg)
#define TRPC_LOG_CRITICAL(msg) TRPC_LOG_MSG(::trpc::Log::critical, msg)

/// @brief stream-like log macros
#define TRPC_LOGGER_TRACE(instance, msg) TRPC_LOGGER_MSG(::trpc::Log::trace, instance, msg)
#define TRPC_LOGGER_DEBUG(instance, msg) TRPC_LOGGER_MSG(::trpc::Log::debug, instance, msg)
#define TRPC_LOGGER_INFO(instance, msg) TRPC_LOGGER_MSG(::trpc::Log::info, instance, msg)
#define TRPC_LOGGER_WARN(instance, msg) TRPC_LOGGER_MSG(::trpc::Log::warn, instance, msg)
#define TRPC_LOGGER_ERROR(instance, msg) TRPC_LOGGER_MSG(::trpc::Log::error, instance, msg)
#define TRPC_LOGGER_CRITICAL(instance, msg) TRPC_LOGGER_MSG(::trpc::Log::critical, instance, msg)

/// @brief stream-like style log macros with conditions
#define TRPC_LOG_TRACE_IF(condition, msg) TRPC_LOG_MSG_IF(::trpc::Log::trace, condition, msg)
#define TRPC_LOG_DEBUG_IF(condition, msg) TRPC_LOG_MSG_IF(::trpc::Log::debug, condition, msg)
#define TRPC_LOG_INFO_IF(condition, msg) TRPC_LOG_MSG_IF(::trpc::Log::info, condition, msg)
#define TRPC_LOG_WARN_IF(condition, msg) TRPC_LOG_MSG_IF(::trpc::Log::warn, condition, msg)
#define TRPC_LOG_ERROR_IF(condition, msg) TRPC_LOG_MSG_IF(::trpc::Log::error, condition, msg)
#define TRPC_LOG_CRITICAL_IF(condition, msg) TRPC_LOG_MSG_IF(::trpc::Log::critical, condition, msg)

#define TRPC_LOGGER_TRACE_IF(instance, condition, msg) TRPC_LOG_MSG_IF(::trpc::Log::trace, instance, condition, msg)
#define TRPC_LOGGER_DEBUG_IF(instance, condition, msg) TRPC_LOG_MSG_IF(::trpc::Log::debug, instance, condition, msg)
#define TRPC_LOGGER_INFO_IF(instance, condition, msg) TRPC_LOG_MSG_IF(::trpc::Log::info, instance, condition, msg)
#define TRPC_LOGGER_WARN_IF(instance, condition, msg) TRPC_LOG_MSG_IF(::trpc::Log::warn, instance, condition, msg)
#define TRPC_LOGGER_ERROR_IF(instance, condition, msg) TRPC_LOG_MSG_IF(::trpc::Log::error, instance, condition, msg)
#define TRPC_LOGGER_CRITICAL_IF(instance, condition, msg) \
  TRPC_LOG_MSG_IF(::trpc::Log::critical, instance, condition, msg)

/// @brief logger instances can be specified to customize the log output.
/// @note  Use case: Separate business logs from framework logs,
///        Different business logs specify different loggers.
///        For example, if remote logs are connected, business logs can be output to remote.
#define TRPC_FLOW_LOG(instance, msg) TRPC_STREAM(instance, ::trpc::Log::info, nullptr, msg)
#define TRPC_FLOW_LOG_EX(context, instance, msg) TRPC_STREAM(instance, ::trpc::Log::info, context, msg)

/// @brief Provides ASSERT that does not invalidate in release mode
/// If x is not true, output a CRITICAL log to the local log and then abort().
/// @note Note:
/// 1. The local log plugin needs to flush CRITICAL logs to disk in real time.
/// (The default log plugin provides guarantees, no additional Settings are required)
/// 2.Referenced flare's log implementation to reduce assembly code generation:
/// a. lambda with __attribute__((noinline, cold)),
/// This will compile the lambda into a separate function in the "cold" section.
/// b. The lambda doesn't need to be exposed, so it doesn't need to be generated
/// Use standard calling convention.
#define TRPC_ASSERT(x)                                 \
  do {                                                 \
    if (__builtin_expect(!(x), 0)) {                   \
      [&]() __attribute__((noinline, cold)) {          \
        TRPC_FMT_CRITICAL("assertion failed: {}", #x); \
        std::abort();                                  \
      }                                                \
      ();                                              \
      __builtin_unreachable();                         \
    }                                                  \
  } while (0)

/// @brief ASSERT is only valid in debug mode.
#ifndef NDEBUG
#define TRPC_DEBUG_ASSERT(x) TRPC_ASSERT(x)
#else
#define TRPC_DEBUG_ASSERT(x) static_cast<void>(0)
#endif

/// @brief User used a log macro with a condition.
/// @note  It is printed only once; in this case,
///        ATOMIC_FLAG_INIT belongs to the macro definition in the system library.
#define TRPC_ONCE()                                               \
  ({                                                              \
    static std::atomic_flag __TRPC_LOG_FLAG__ = ATOMIC_FLAG_INIT; \
    (!__TRPC_LOG_FLAG__.test_and_set(std::memory_order_relaxed)); \
  })

/// @brief You want to log but don't want to log so much that it degrades performance.
///        Use the conditional macro operator.
///        Set this log to be printed at most once every c times, the first time must be fired.
#define TRPC_EVERY_N(n)                                                            \
  ({                                                                               \
    TRPC_ASSERT((n) > 0 && "should not be less than 1");                           \
    static std::atomic<uint64_t> __TRPC_LOG_OCCURENCE_N__{(n)};                    \
    (__TRPC_LOG_OCCURENCE_N__.fetch_add(1, std::memory_order_relaxed) % (n) == 0); \
  })

/// @brief Set the condition for N milliseconds.
/// @note  This macro is typically used for the if condition of user log macro output,
/// where ATOMIC_FLAG_INIT belongs to the macro definition in the system library.
#define TRPC_WITHIN_N(ms)                                                                                     \
  ({                                                                                                          \
    TRPC_ASSERT((ms) > 0 && "should not be less than 1");                                                     \
    bool __TRPC_LOG_WRITEABLE__ = false;                                                                      \
    static std::chrono::time_point<std::chrono::steady_clock> __TRPC_TIME_POINT__;                            \
    static std::atomic_flag __TRPC_LOG_CONDITION_FLAG__ = ATOMIC_FLAG_INIT;                                   \
    if (!__TRPC_LOG_CONDITION_FLAG__.test_and_set(std::memory_order_acquire)) {                               \
      auto __TRPC_NOWS__ = std::chrono::steady_clock::now();                                                  \
      int64_t __TRPC_DIFF_N__ =                                                                               \
          std::chrono::duration_cast<std::chrono::milliseconds>(__TRPC_NOWS__ - __TRPC_TIME_POINT__).count(); \
      if (__TRPC_DIFF_N__ > ms) {                                                                             \
        __TRPC_TIME_POINT__ = __TRPC_NOWS__;                                                                  \
        __TRPC_LOG_WRITEABLE__ = true;                                                                        \
      }                                                                                                       \
      __TRPC_LOG_CONDITION_FLAG__.clear(std::memory_order_release);                                           \
    }                                                                                                         \
    __TRPC_LOG_WRITEABLE__;                                                                                   \
  })

/// @brief Print only once.
#define TRPC_FMT_INFO_ONCE(format, args...) TRPC_FMT_INFO_IF(TRPC_ONCE(), format, ##args)
#define TRPC_FMT_WARN_ONCE(format, args...) TRPC_FMT_WARN_IF(TRPC_ONCE(), format, ##args)
#define TRPC_FMT_ERROR_ONCE(format, args...) TRPC_FMT_ERROR_IF(TRPC_ONCE(), format, ##args)
#define TRPC_FMT_CRITICAL_ONCE(format, args...) TRPC_FMT_CRITICAL_IF(TRPC_ONCE(), format, ##args)

/// @brief Print it every N times.
#define TRPC_FMT_INFO_EVERY_N(N, format, args...) TRPC_FMT_INFO_IF(TRPC_EVERY_N(N), format, ##args)
#define TRPC_FMT_WARN_EVERY_N(N, format, args...) TRPC_FMT_WARN_IF(TRPC_EVERY_N(N), format, ##args)
#define TRPC_FMT_ERROR_EVERY_N(N, format, args...) TRPC_FMT_ERROR_IF(TRPC_EVERY_N(N), format, ##args)
#define TRPC_FMT_CRITICAL_EVERY_N(N, format, args...) TRPC_FMT_CRITICAL_IF(TRPC_EVERY_N(N), format, ##args)

/// @brief Print every second.
#define TRPC_FMT_INFO_EVERY_SECOND(format, args...) TRPC_FMT_INFO_IF(TRPC_WITHIN_N(1000), format, ##args)
#define TRPC_FMT_WARN_EVERY_SECOND(format, args...) TRPC_FMT_WARN_IF(TRPC_WITHIN_N(1000), format, ##args)
#define TRPC_FMT_ERROR_EVERY_SECOND(format, args...) TRPC_FMT_ERROR_IF(TRPC_WITHIN_N(1000), format, ##args)
#define TRPC_FMT_CRITICAL_EVERY_SECOND(format, args...) TRPC_FMT_CRITICAL_IF(TRPC_WITHIN_N(1000), format, ##args)
