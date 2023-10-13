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

// To avoid circular dependency chain, we implement "basic" logging facility
// here, for other "low-level" components to use.

#include <atomic>
#include <string>
#include <utility>

#include "fmt/core.h"
#include "fmt/format.h"
#include "fmt/ostream.h"
#include "fmt/std.h"

#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

// glog's CHECK_OP does not optimize well by GCC 8.2. `google::CheckOpString`
// tries to hint the compiler, but the compiler does not seem to recognize that.
// In the meantime, `CHECK_OP` produces really large amount of code which can
// hurt performance. Our own implementation avoid these limitations.
#define TRPC_CHECK(expr, ...) TRPC_INTERNAL_DETAIL_LOGGING_CHECK(expr, ##__VA_ARGS__)
#define TRPC_CHECK_EQ(val1, val2, ...) \
  TRPC_INTERNAL_DETAIL_LOGGING_CHECK_OP(_EQ, ==, val1, val2, ##__VA_ARGS__)  // `##` is GNU extension.
#define TRPC_CHECK_NE(val1, val2, ...) TRPC_INTERNAL_DETAIL_LOGGING_CHECK_OP(_NE, !=, val1, val2, ##__VA_ARGS__)
#define TRPC_CHECK_LE(val1, val2, ...) TRPC_INTERNAL_DETAIL_LOGGING_CHECK_OP(_LE, <=, val1, val2, ##__VA_ARGS__)
#define TRPC_CHECK_LT(val1, val2, ...) TRPC_INTERNAL_DETAIL_LOGGING_CHECK_OP(_LT, <, val1, val2, ##__VA_ARGS__)
#define TRPC_CHECK_GE(val1, val2, ...) TRPC_INTERNAL_DETAIL_LOGGING_CHECK_OP(_GE, >=, val1, val2, ##__VA_ARGS__)
#define TRPC_CHECK_GT(val1, val2, ...) TRPC_INTERNAL_DETAIL_LOGGING_CHECK_OP(_GT, >, val1, val2, ##__VA_ARGS__)
#define TRPC_CHECK_NEAR(val1, val2, margin, ...)             \
  do {                                                       \
    TRPC_CHECK_LE((val1), (val2) + (margin), ##__VA_ARGS__); \
    TRPC_CHECK_GE((val1), (val2) - (margin), ##__VA_ARGS__); \
  } while (0)

// For `DCHECK`s, we don't optimize them much, as they're totally optimized away
// in release build anyway.
#ifndef NDEBUG
#define TRPC_DCHECK(expr, ...) TRPC_CHECK(expr, ##__VA_ARGS__)
#define TRPC_DCHECK_EQ(expr1, expr2, ...) TRPC_CHECK_EQ(expr1, expr2, ##__VA_ARGS__)
#define TRPC_DCHECK_NE(expr1, expr2, ...) TRPC_CHECK_NE(expr1, expr2, ##__VA_ARGS__)
#define TRPC_DCHECK_LE(expr1, expr2, ...) TRPC_CHECK_LE(expr1, expr2, ##__VA_ARGS__)
#define TRPC_DCHECK_GE(expr1, expr2, ...) TRPC_CHECK_GE(expr1, expr2, ##__VA_ARGS__)
#define TRPC_DCHECK_LT(expr1, expr2, ...) TRPC_CHECK_LT(expr1, expr2, ##__VA_ARGS__)
#define TRPC_DCHECK_GT(expr1, expr2, ...) TRPC_CHECK_GT(expr1, expr2, ##__VA_ARGS__)
#define TRPC_DCHECK_NEAR(expr1, expr2, margin, ...) TRPC_CHECK_NEAR(expr1, expr2, margin, ##__VA_ARGS__)
#else
#define TRPC_DCHECK(expr, ...)
#define TRPC_DCHECK_EQ(expr1, expr2, ...)
#define TRPC_DCHECK_NE(expr1, expr2, ...)
#define TRPC_DCHECK_LE(expr1, expr2, ...)
#define TRPC_DCHECK_GE(expr1, expr2, ...)
#define TRPC_DCHECK_LT(expr1, expr2, ...)
#define TRPC_DCHECK_GT(expr1, expr2, ...)
#define TRPC_DCHECK_NEAR(expr1, expr2, margin, ...)
#endif

#define TRPC_PCHECK(expr, ...) TRPC_INTERNAL_DETAIL_LOGGING_PCHECK(expr, ##__VA_ARGS__)

#define TRPC_CHECK_LOG(...)                                                                     \
  do {                                                                                          \
    TRPC_FMT(trpc::log::kTrpcLogCacheStringDefault, trpc::Log::Level::critical, ##__VA_ARGS__); \
    [&]() TRPC_INTERNAL_DETAIL_LOGGING_ATTRIBUTE_NOINLINE_COLD {                                \
      trpc::log::Destroy();                                                                     \
      std::abort();                                                                             \
      TRPC_UNREACHABLE();                                                                       \
    }();                                                                                        \
  } while (0)

#ifdef _MSC_VER
#define TRPC_UNREACHABLE()                    \
  do {                                        \
    std::cerr << "Unreachable." << std::endl; \
  } while (0)
#else
#define TRPC_UNREACHABLE()                    \
  do {                                        \
    std::cerr << "Unreachable." << std::endl; \
    __builtin_unreachable();                  \
  } while (0)
#endif

///////////////////////////////////////
// Implementation goes below.        //
///////////////////////////////////////

// C++ attribute won't work here.
//
// @sa: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89640,
#define TRPC_INTERNAL_DETAIL_LOGGING_ATTRIBUTE_NOINLINE_COLD __attribute__((noinline, cold))

/// @private
namespace trpc::detail::log {

// Marked as noexcept. Throwing in formatting log is likely a programming
// error.
template <class S, class... T>
inline std::string FormatLogOpt([[maybe_unused]] int, const S& fmt, T&&... args) noexcept {
  return fmt::format(fmt::runtime(fmt), std::forward<T>(args)...);
}
// NOCA:readability/casting(false positive from code scanning tool.)
inline std::string FormatLogOpt([[maybe_unused]] int) noexcept { return ""; }

template <class S, class... T>
inline std::string FormatLog(const S& fmt, T&&... args) noexcept {
  return FormatLogOpt(0, fmt, std::forward<T>(args)...);
}
inline std::string FormatLog() noexcept { return ""; }

template <class T>
T&& OpVal(T&& val) {
  return std::forward<T>(val);
}

template <class T>
void* OpVal(T* val) {
  return static_cast<void*>(val);
}

std::string StrError(int err);

}  // namespace trpc::detail::log

// Clang 10 has not implemented P1381R1 yet, therefore the "cold lambda" trick
// won't work quite right when structured binding identifiers are accessed when
// evaluating log message.
#if defined(__clang__) && __clang__ <= 10

#define TRPC_INTERNAL_DETAIL_LOGGING_CHECK(expr, ...)                                                      \
  do {                                                                                                     \
    if (TRPC_UNLIKELY(!(expr))) {                                                                          \
      TRPC_CHECK_LOG("Check failed: " #expr " {}", trpc::detail::log::FormatLogOpt(0, ##__VA_ARGS__)); \
    }                                                                                                      \
  } while (0)

#define TRPC_INTERNAL_DETAIL_LOGGING_CHECK_OP(name, op, val1, val2, ...)                                             \
  do {                                                                                                               \
    /* `google::GetReferenceableValue` triggers `-Wmaybe-uninitialized`, */                                          \
    /* Not sure about the reason though. */                                                                          \
    auto&& TRPC_anonymous_x = (val1);                                                                                \
    auto&& TRPC_anonymous_y = (val2);                                                                                \
    if (TRPC_UNLIKELY(!(TRPC_anonymous_x op TRPC_anonymous_y))) {                                                    \
      TRPC_CHECK_LOG("Check failed: " #val1 "({}) " #op " " #val2 "({}) {}",                                         \
                     trpc::detail::log::OpVal(TRPC_anonymous_x), trpc::detail::log::OpVal(TRPC_anonymous_y), \
                     trpc::detail::log::FormatLogOpt(0, ##__VA_ARGS__));                                         \
    }                                                                                                                \
  } while (0)

#define TRPC_INTERNAL_DETAIL_LOGGING_PCHECK(expr, ...)                                          \
  do {                                                                                          \
    if (TRPC_UNLIKELY(!(expr))) {                                                               \
      TRPC_CHECK_LOG("[{}] Check failed: " #expr " {}", trpc::detail::log::StrError(errno), \
                     trpc::detail::log::FormatLogOpt(0, ##__VA_ARGS__));                    \
    }                                                                                           \
  } while (0)

#else

#define TRPC_INTERNAL_DETAIL_LOGGING_CHECK(expr, ...)                                                        \
  do {                                                                                                       \
    if (TRPC_UNLIKELY(!(expr))) {                                                                            \
      /* Attribute `noreturn` is not applicable to lambda, unfortunately. */                                 \
      [&]() TRPC_INTERNAL_DETAIL_LOGGING_ATTRIBUTE_NOINLINE_COLD {                                           \
        TRPC_CHECK_LOG("Check failed: " #expr " {}", trpc::detail::log::FormatLogOpt(0, ##__VA_ARGS__)); \
      }();                                                                                                   \
    }                                                                                                        \
  } while (0)

#define TRPC_INTERNAL_DETAIL_LOGGING_CHECK_OP(name, op, val1, val2, ...)                                               \
  do {                                                                                                                 \
    /* `google::GetReferenceableValue` triggers `-Wmaybe-uninitialized`, */                                            \
    /* Not sure about the reason though. */                                                                            \
    auto&& TRPC_anonymous_x = (val1);                                                                                  \
    auto&& TRPC_anonymous_y = (val2);                                                                                  \
    if (TRPC_UNLIKELY(!(TRPC_anonymous_x op TRPC_anonymous_y))) {                                                      \
      [&]() TRPC_INTERNAL_DETAIL_LOGGING_ATTRIBUTE_NOINLINE_COLD {                                                     \
        TRPC_CHECK_LOG("Check failed: " #val1 "({}) " #op " " #val2 "({}) {}",                                         \
                       trpc::detail::log::OpVal(TRPC_anonymous_x), trpc::detail::log::OpVal(TRPC_anonymous_y), \
                       trpc::detail::log::FormatLogOpt(0, ##__VA_ARGS__));                                         \
      }();                                                                                                             \
    }                                                                                                                  \
  } while (0)

#define TRPC_INTERNAL_DETAIL_LOGGING_PCHECK(expr, ...)                                            \
  do {                                                                                            \
    if (TRPC_UNLIKELY(!(expr))) {                                                                 \
      [&]() TRPC_INTERNAL_DETAIL_LOGGING_ATTRIBUTE_NOINLINE_COLD {                                \
        TRPC_CHECK_LOG("[{}] Check failed: " #expr " {}", trpc::detail::log::StrError(errno), \
                       trpc::detail::log::FormatLogOpt(0, ##__VA_ARGS__));                    \
      }();                                                                                        \
    }                                                                                             \
  } while (0)

#endif
