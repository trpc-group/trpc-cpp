//
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/casting.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include "trpc/util/internal/demangle.h"
#include "trpc/util/internal/index_alloc.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

/// @brief Inspired by `llvm/Support/Casting.h`.
/// @sa: https://www.llvm.org/docs/HowToSetUpLLVMStyleRTTI.html (design)
/// @sa: https://llvm.org/doxygen/Casting_8h.html (source code)
/// Our design deviates from std's in:
///   - `(dyn_)cast` does not accept `nullptr`, use `(dyn_)cast_or_null` instead.
///     (LLVM does this, which is good, since `nullptr` shouldn't appear in most cases.)
///   - In cases when conversion failure is fatal, `cast` should be used in place
///     of `dyn_cast`. We follow LLVM's design here to be more clear about the
///     semantics. (In std's convention, whether conversion failure is fatal
///     depends on "target type". That's a little vague IMO.)
///   - We follow LLVM's convention, in that `(dyn_)cast(_or_null)` accepts a
///     "normal" type (instead of a reference or pointer type) as its template
///     parameter. The keyword `dynamic_cast` accepts either `T*` or `T&`. This is,
///     to say the least, inconsistent with std's convention. However, the
///     inconsistency also exists in the Standard Library (@sa: `xxx_pointer_cast`,
///     which accept "normal" type, too), so we tolerate this inconsistency.
/// @note Usage:
/// We cannot magically tell if a given reference or pointer is of excepted
/// runtime type. It's your responsibility to provide such facility.
///
/// The contract is that for each type `T`, you must implement do one of the followings:
///   - If you always cast objects to their exact runtime type (not ever casting
///     them to some type they inherited from) when doing down-casting, you can
///     inherit your type from `ExactMatchCastable` and call `SetRuntimeType(this,
///     kRuntimeType<T>)` in constructor. (@sa: `ExactMatchCastable`.)
///     You can still use `dynamic_cast` or specialize `CastingTraits<T>` for
///     certain types to down-cast to their base classes in this case.
///   - Provide a static method of signature `bool T::classof(const U& val)` in
///     `T`, which returns `true` if `val` can be casted to `T` (i.e., `val`'s
///     runtime type is (or is a subclass) of `T`.). You can use `Castable` (@sa:
///     `Castable`) to simplify your work a bit.
///   - Specialize traits type `CastingTraits<T>`, which in turn, provides static
///     method doing the same thing as `T::classof`, with signature `bool
///     RuntimeTypeCheck(const U& val)`. Compared to providing `classof`, this
///     approach might seem to be more non-intrusive. However, this can be more
///     verbose, and in most cases, it's an overkill.
/// @note Implementation note:
///     `bool T::classof(const U& val)` tests if `val`'s runtime type is (or inherits
///     from) `T`, by whatever means you deem fit. We use this information is to
///     implement methods listed in this file correctly.
///
///   One way to do this is tagging each instance with its real type, e.g.:
///
///   ```
///   struct Base {
///     enum { kA, kB } type;
///
///     // For base class, there's no need to provide `classof(...)`. Casting
///     // from derived types to base is always allowed.
///   };
///
///   struct A : Base {
///     A() { type = kA; }
///
///     static bool classof(const Base& val) {
///       // `B` inherits from `A`. Consequently, every instance of `B` is also
///       // an instance of `A`. (@sa: `B`.)
///       return val.type == kA || val.type == kB;
///     }
///   };
///
///   struct B : A {  // Inherits from `A`.
///     B() { type = kB; }
///
///     static bool classof(const Base& val) { return val.type == kB; }
///   };
///   ```

namespace trpc::internal {

/// @brief In case it's inconvenient for you to add `classof` to `T`, you can specialize `CastingTraits<T>` instead.
template <class T, class = void>
struct CastingTraits {
  // The default implementation delegate itself to `T::classof`.
  template <class U>
  static bool RuntimeTypeCheck(const U& val) {
    return T::classof(val);
  }
};

/// @brief If you only need an `int` to distinguish runtime types, you can simply
///        inherit from this base class. This should ease your work a little.
/// @note After inheriting from this base, you can use following variable(s) to ...:
///       - `kRuntimeType<T>`: For each type, a distinct index is assigned to this
///         (template) variable.
///@note ... and following methods (do NOT qualify the call, they're found via ADL) to:
///       - `SetRuntimeType(Castable<T>*, int)`: Set instance's runtime type (usually
///           done in class's constructor).
///       - `GetRuntimeType(const Castable<T>&)`: Get what's set by `SetRuntimeType`.
/// @note that not all (perhaps pure) abstract classes should be made `Castable`.
///       If the base class is intended to serve as an interface, you really should not
///       cast instances to concrete class (you should program based on interface
///       instead). Therefore, do not inherit from this base in such cases.
class Castable {
  class Dummy {};

 public:
  // The last argument is not used by the two friend methods. They're there to
  // prevent ambiguities in case there's another free-standing method with the
  // same name.
  friend void SetRuntimeType(Castable* ptr, int type, Dummy = {}) { ptr->type_ = type; }
  friend int GetRuntimeType(const Castable& val, Dummy = {}) { return val.type_; }

 protected:
  // Allow `CastingTraits` to access `kRuntimeType<T>` below.
  template <class, class>
  friend struct CastingTraits;

  ~Castable() = default;

  // Shorthand for `SetRuntimeType(this, kRuntimeType<T>)`.
  template <class T>
  void SetRuntimeTypeTo() {
    SetRuntimeType(this, kRuntimeType<T>);
  }

  template <class T>
  inline static const int kRuntimeType = IndexAlloc::For<Castable>()->Next();

 private:
  // Or `TypeIndex type_` here? `TypeIndex` seems to be more powerful, and
  // should perform equally well.
  std::int_fast32_t type_ = kRuntimeType<Castable>;
};

/// @brief If you always cast objects to their *exact* runtime type (i.e., not some
///        intermediate type they inherited), then the framework can helps you to tell
/// @note if the casting should be allowed:
///       - For up-casting, it's always done at compile-time, therefore it's supported implicitly.
///       - For down-casting, since you're casting to the exact runtime type of the
///         object, it's simply a matter of comparing `GetRuntimeType(object)` to `kRuntimeType<To>`.
/// Therefore, when possible, by inheriting from this base, you only have to call `SetRuntimeType(this,
/// kRuntimeType<T>)` in constructor, and everything else is done by the framework for you.
class ExactMatchCastable : public Castable {
 protected:
  ~ExactMatchCastable() = default;
};

template <class T>
struct CastingTraits<T, std::enable_if_t<std::is_base_of_v<ExactMatchCastable, T>>> {
  // Test if `object`'s runtime type is exactly `T` (not something inherited
  // from `T` or anything else).
  static bool RuntimeTypeCheck(const ExactMatchCastable& object) {
    return GetRuntimeType(object) == Castable::kRuntimeType<T>;
  }
};

namespace casting::detail {

/// @brief Handles the cases when `U` is base of `T`.
/// @note In this case we need to test `val`'s type at runtime.
template <class T, class U, class = std::enable_if_t<std::is_base_of_v<U, T> && !std::is_same_v<U, T>>>
inline bool RuntimeTypeCheck(const U& val) {
  return CastingTraits<T>::RuntimeTypeCheck(val);
}

/// @brief Handle the case when `T` is (or is base of) `val`'s *compile-time* type.
/// @note that `val`'s runtime type can only be either the same as its
///       compile-time type, or a more derived type. Either way, we're safe.
template <class T>
inline bool RuntimeTypeCheck(const T& val) {
  return true;
}

template <class T, class U>
using casted_type_t = std::conditional_t<std::is_const_v<U>, const T*, T*>;

template <class T, class U>
[[noreturn, gnu::noinline]] void InvalidCast(U* ptr) {
  bool really_castable;  // Set if the cast would succeed were `dynamic_cast`
                         // used instead.

  // Performance does not matter here, as we're going to crash the whole program
  // anyway.
  if constexpr (std::has_virtual_destructor_v<T>) {
    really_castable = dynamic_cast<const T*>(ptr);
  } else {
    really_castable = typeid(T) == typeid(*ptr);  // Not very accurate.
  }

  if (really_castable) {
    TRPC_FMT_CRITICAL(
        "Casting to type [{}] failed. However, the C++ runtime reports that "
        "you're indeed casting to the (right) runtime type of the object. This "
        "can happen when either: 1) you haven't initialize object's runtime "
        "type correctly (e.g. via `SetRuntimeTypeTo` if you're inheriting from "
        "`ExactMatchCast`), or 2) the implementation of your `classof` is "
        "incorrect.",
        GetTypeName<T>());
  } else {
    TRPC_FMT_CRITICAL(
        "Invalid cast: Runtime type [{}] expected, got [{}]. If you believe "
        "this is an error, check if your `classof` is implemented correctly",
        GetTypeName<T>(), GetTypeName(*ptr));
  }
  __builtin_unreachable();
}

}  // namespace casting::detail

/// @brief Test if `val` is *actually* (i.e., its runtime type is tested) an instance of `T`.
template <class T, class U, class = std::enable_if_t<!std::is_pointer_v<U>>>
inline bool isa(const U& val) {
  return casting::detail::RuntimeTypeCheck<T>(val);
}

template <class T, class U>
inline bool isa(const U* val) {
  return isa<T>(*val);
}

/// @brief Cast `ptr` to `T*` if runtime type of what it points to is (or inherits from) `T`.
///        Returns `nullptr` otherwise.
/// @note Passing `nullptr` to this method results in U.B. (@sa: `dyn_cast_or_null`.)
template <class T, class U, class R = casting::detail::casted_type_t<T, U>>
R dyn_cast(U* ptr) {
  return isa<T>(ptr) ? static_cast<R>(ptr) : nullptr;
}

/// @brief Same as `T* dyn_cast(U*)`, but accepts a reference. Returns pointer.
template <class T, class U>
inline auto dyn_cast(U& val) {
  return dyn_cast<T>(&val);
}

/// @brief Same as `dyn_cast`, except that this one handles `nullptr` gracefully (by returning `nullptr`.).
template <class T, class U>
inline auto dyn_cast_or_null(U* ptr) {
  return ptr ? dyn_cast<T>(ptr) : nullptr;
}

/// @brief Cast `val` to `T*`. Runtime type of `val` must be (or inherit from) `T`.
/// @note Passing `nullptr` to this method results in U.B. (@sa: `cast_or_null`.)
template <class T, class U, class R = casting::detail::casted_type_t<T, U>>
inline R cast(U* ptr) {
  if (TRPC_LIKELY(isa<T>(ptr))) {
    return static_cast<R>(ptr);
  }
  casting::detail::InvalidCast<T>(ptr);
}

/// @brief Same as `cast<T>(U*)`, except that this one accepts a reference instead.
template <class T, class U>
inline auto cast(U& val) {
  return cast<T>(&val);
}

/// @brief Same as `cast<T>(U*)`, while `nullptr` is handled gracefully.
template <class T, class U>
inline auto cast_or_null(U* ptr) {
  return ptr ? cast<T>(ptr) : nullptr;
}

}  // namespace trpc::internal
