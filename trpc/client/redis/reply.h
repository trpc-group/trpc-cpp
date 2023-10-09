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

#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace trpc {

namespace redis {

struct StringReplyMarker {};
struct StatusReplyMarker {};
struct ErrorReplyMarker {};
struct ArrayReplyMarker {};
struct IntegerReplyMarker {};
struct NilReplyMarker {};
struct InvalidReplyMarker {};

/// @brief redis reply wrapper class
struct Reply {
 public:
  enum Type {
    NONE,
    STRING,
    STATUS,
    ERROR,
    INTEGER,
    NIL,
    ARRAY,
    INVALID,
  };

  union Any {
    Any() {}
    ~Any() {}
    Any(std::string&& value) : value_(std::move(value)) {}
    Any(int64_t integer) : integer_(integer) {}
    Any(std::vector<Reply>&& array) : array_(std::move(array)) {}

    std::string value_;

    int64_t integer_{0};

    std::vector<Reply> array_;
  };

  Type type_ = Type::NONE;
  Any u_;
  bool has_value_ = false;
  uint32_t serial_no_ = 0;

  Reply() noexcept : type_(Type::NONE), has_value_(false) {}

  Reply(StringReplyMarker, std::basic_string<char>&& s)
      : type_(Type::STRING), u_(std::move(s)), has_value_(true), serial_no_(0) {}

  Reply(ArrayReplyMarker, std::vector<Reply>&& a) : type_(Type::ARRAY), u_(std::move(a)), has_value_(true) {}

  Reply(IntegerReplyMarker, int64_t i) : type_(Type::INTEGER), u_(i), has_value_(true), serial_no_(0) {}

  explicit Reply(NilReplyMarker) : type_(Type::NIL), has_value_(true), serial_no_(0) {}

  Reply(StatusReplyMarker, std::basic_string<char>&& s)
      : type_(Type::STATUS), u_(std::move(s)), has_value_(true), serial_no_(0) {}

  Reply(ErrorReplyMarker, std::basic_string<char>&& s)
      : type_(Type::ERROR), u_(std::move(s)), has_value_(true), serial_no_(0) {}

  explicit Reply(InvalidReplyMarker) : type_(Type::INVALID), serial_no_(0) {}

  [[gnu::always_inline]] Reply(Reply&& x) noexcept
      : type_(x.type_), has_value_(x.has_value_), serial_no_(x.serial_no_) {
    switch (type_) {
      case Type::NONE:
        break;
      case Type::STRING:
      case Type::STATUS:
      case Type::ERROR:
        new (&u_.value_) std::basic_string<char>(std::move(x.u_.value_));
        x.u_.value_.~basic_string();
        break;
      case Type::INTEGER:
        u_.integer_ = x.u_.integer_;
        break;
      case Type::ARRAY:
        new (&u_.array_) std::vector<Reply>(std::move(x.u_.array_));
        x.u_.array_.~vector();
        break;
      case Type::NIL:
      case Type::INVALID:
        break;
      default:
        break;
    }

    x.type_ = Type::INVALID;
  }

  [[gnu::always_inline]] ~Reply() noexcept {
    switch (type_) {
      case Type::STRING:
      case Type::STATUS:
      case Type::ERROR:
        if (has_value_) {
          u_.value_.~basic_string();
        }
        break;
      case Type::INTEGER:
        break;
      case Type::ARRAY:
        if (has_value_) {
          u_.array_.~vector();
        }
        break;
      case Type::NONE:
      case Type::NIL:
      case Type::INVALID:
        break;
      default:
        break;
    }
  }

  Reply(const Reply& x) noexcept : type_(x.type_), has_value_(x.has_value_), serial_no_(x.serial_no_) {
    switch (type_) {
      case Type::NONE:
        break;
      case Type::STRING:
      case Type::STATUS:
      case Type::ERROR:
        new (&u_.value_) std::basic_string<char>(x.u_.value_);
        break;
      case Type::INTEGER:
        u_.integer_ = x.u_.integer_;
        break;
      case Type::ARRAY:
        new (&u_.array_) std::vector<Reply>(x.u_.array_);
        break;
      case Type::NIL:
      case Type::INVALID:
        break;
      default:
        break;
    }
  }

  Reply& operator=(Reply&& x) noexcept {
    if (this != &x) {
      this->~Reply();
      new (this) Reply(std::move(x));
    }
    return *this;
  }

  Reply& operator=(const Reply& x) noexcept {
    if (this != &x) {
      this->~Reply();
      new (this) Reply(x);
    }
    return *this;
  }

  inline void Set(StringReplyMarker, const char* s, size_t len, size_t capacity) {
    has_value_ = true;
    new (&u_.value_) std::string();
    u_.value_.reserve(capacity);
    u_.value_.append(s, len);
  }

  inline void Set(StringReplyMarker, const char* s, size_t len) {
    has_value_ = true;
    new (&u_.value_) std::string();
    if (len < 1) {
      return;
    }
    u_.value_.reserve(len);
    u_.value_.append(s, len);
  }

  inline void Set(StatusReplyMarker, const char* s, size_t len, size_t capacity) {
    has_value_ = true;
    new (&u_.value_) std::string();
    u_.value_.reserve(capacity);
    u_.value_.append(s, len);
  }

  inline void Set(StatusReplyMarker, const char* s, size_t len) {
    has_value_ = true;
    new (&u_.value_) std::string(s, s + len);
  }

  inline void Set(ErrorReplyMarker, const char* s, size_t len, size_t capacity) {
    has_value_ = true;
    new (&u_.value_) std::string();
    u_.value_.reserve(capacity);
    u_.value_.append(s, len);
  }

  inline void Set(ErrorReplyMarker, const char* s, size_t len) {
    has_value_ = true;
    new (&u_.value_) std::string(s, s + len);
  }

  inline void Set(IntegerReplyMarker, int64_t i) { u_.integer_ = i; }
  inline void Set(ArrayReplyMarker) {
    has_value_ = true;
    new (&u_.array_) std::vector<Reply>();
  }
  inline void Set(NilReplyMarker) { type_ = Type::NIL; }
  inline void Set(InvalidReplyMarker) { type_ = Type::INVALID; }

  inline void AppendString(const char* s, size_t len) { u_.value_.append(s, len); }

  inline bool IsNone() const { return type_ == Type::NONE; }
  inline bool IsNil() const { return type_ == Type::NIL; }
  inline bool IsString() const { return type_ == Type::STRING; }
  inline bool IsStatus() const { return type_ == Type::STATUS; }
  inline bool IsError() const { return type_ == Type::ERROR; }
  inline bool IsArray() const { return type_ == Type::ARRAY; }
  inline bool IsInteger() const { return type_ == Type::INTEGER; }
  inline bool IsInvalid() const { return type_ == Type::INVALID; }

  /// @brief Get reply as string ONLY when type is in[string/status/error]
  inline const std::basic_string<char>& GetString() const { return u_.value_; }

  inline int64_t GetInteger() const { return u_.integer_; }
  inline const std::vector<Reply>& GetArray() const { return u_.array_; }

  /// @brief Get reply as string ONLY when type is string when need for high performance
  /// It will use std::move the move this reply,so DO NOT invoke repeatly
  inline int GetString(std::string& value) {
    if (has_value_ && type_ == Type::STRING) {
      if (!u_.value_.empty()) {
        value = std::move(u_.value_);
      }
      has_value_ = false;
      return 0;
    }
    return -1;
  }

  /// @brief Get reply as Array when need for high performance
  /// It will use std::move the move this reply,so DO NOT invoke repeatly
  inline int GetArray(std::vector<Reply>& value) {
    if (has_value_ && type_ == Type::ARRAY) {
      if (!u_.array_.empty()) {
        value = std::move(u_.array_);
      }
      has_value_ = false;
      return 0;
    }
    return -1;
  }

  inline const std::basic_string<char> TypeToString() const {
    switch (type_) {
      case Type::NONE:
        return "none";
      case Type::NIL:
        return "nil";
      case Type::STRING:
        return "string";
      case Type::ARRAY:
        return "array";
      case Type::INTEGER:
        return "integer";
      case Type::STATUS:
        return "status";
      case Type::ERROR:
        return "error";
      case Type::INVALID:
        return "invalid";
      default:
        return "unkonwn";
    }
  }

  inline Type GetType() const { return type_; }
};
}  // namespace redis

}  // namespace trpc

namespace std {
// Streams a Message to an ostream.
inline std::ostream& operator<<(std::ostream& os, const trpc::redis::Reply& reply) {
  switch (reply.type_) {
    case trpc::redis::Reply::STRING:
      os << "<string> " << reply.GetString();
      break;
    case trpc::redis::Reply::STATUS:
      os << "<status> " << reply.GetString();
      break;
    case trpc::redis::Reply::ERROR:
      os << "<error> " << reply.GetString();
      break;
    case trpc::redis::Reply::ARRAY:
      os << "<array> " << reply.GetArray().size() << "[";
      for (auto& item : reply.GetArray()) {
        os << item;
      }
      os << "]";
      break;
    case trpc::redis::Reply::NONE:
      os << "<none> ";
      break;
    case trpc::redis::Reply::NIL:
      os << "<nil> ";
      break;
    case trpc::redis::Reply::INTEGER:
      os << "<integer> " << reply.GetInteger();
      break;
    default:
      os << "<unknown>";
      break;
  }
  return os;
}
}  // namespace std
