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

// General purpose random number generating methods. They are drop-in
// replacements for `rand` / `rand_r`.
//
// CAUTION: NOTHING HERE SHOULD BE CONSIDERED CRYPTOGRAPHICALLY SECURE. FOR
// CRYPTOGRAPHIC PURPOSE, USE OpenSSL INSTEAD.

#pragma once

#include <cstdint>
#include <memory>
#include <random>
#include <type_traits>

namespace trpc {

namespace detail {

/// @brief Thread local random engine.
/// @note `std::mt19937_64` performs equally well as `std::mt19937` on x86_64. So we
///       don't bother using `std::mt19937` for smaller types.
/// @note We export this method into `gdt::random` (instead of keeping it in some
///       private namespace) so that people who randomly need an engine could directly
///       use this one, rather than declaring their own.
inline std::mt19937_64& Engine() {
  // This could be quite tricky.
  //
  // Ideally we'd like to initialize the PRNG engine's internal state with raw
  // bits from `std::random_device`. However, per spec, the engine only accepts
  // a single number, or a `SeedSequence`, which is not satisfied by
  // `std::random-device`, in its constructor. In the meantime, by using
  // `std::seed_seq`, we potentially degrade our randomness.
  //
  // It would be the best if the Standard requires `std::random_device` to
  // satisfy `SeedSequence`, or (better yet), allows `std::mt19937` to accept a
  // `std::random_device`.
  //
  // More detailed discussion about the initialization of PRNG engine could be
  // found at:
  //
  //   - https://stackoverflow.com/questions/15509270
  //   - http://www.pcg-random.org/posts/cpp-seeding-surprises.html
  //   - https://www.reddit.com/r/cpp/comments/31857s
  //
  // Anyway, below is a more-or-less acceptable initialization, per the SO
  // answer linked above.
  thread_local auto engine = [] {
    std::random_device r;
    std::seed_seq seeds{r(), r(), r(), r(), r(), r(), r(), r()};
    // Use unique_ptr to avoid much space in TLS.
    auto engine = std::make_unique<std::mt19937_64>(seeds);
    return engine;
  }();
  return *engine;
}

}  // namespace detail

/// @brief Generate a random value of type `T`. `T` may not be floating point types.
/// @return A random value in range [std::numeric_limits<T>::min(),std::numeric_limits<T>::max()] (both inclusive).
template <class T = std::uint64_t, class = std::enable_if_t<std::is_integral_v<T>>>
T Random() {
  return detail::Engine()();
}

/// @brief Generate a random value in range [min, max] (inclusive) of type `T`.
/// @param min the min value
/// @param max the max value
/// @return A random value generated are distributed uniformly.
/// @note `SimpleRandom` performs slightly better if multiple random values are needed.
/// @sa: //common/random/simple_random.h.
template <class T>
T Random(T min, T max) {
  using Distribution = std::conditional_t<std::is_floating_point_v<T>, std::uniform_real_distribution<T>,
                                          std::uniform_int_distribution<T>>;
  Distribution dist(min, max);  // May NOT be `thread_local` as `min` / `max`
                                // could be different across calls.
  return dist(detail::Engine());
}

/// @brief Shorthand for `Random(0, max)`.
/// @param max the max value
/// @return A random value
template <class T>
T Random(T max) {
  return Random<T>(0, max);
}

}  // namespace trpc
