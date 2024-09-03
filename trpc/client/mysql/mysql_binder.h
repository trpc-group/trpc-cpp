#pragma once

#include <mysql/mysql.h>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>
#include <iostream>

#define BIND_POINTER_CAST(v) (const_cast<void*>(static_cast<const void*>(v)))

namespace trpc::mysql {

// ***********
// Wrap the MYSQL_TIME
// ***********

struct mysql_time
{
  MYSQL_TIME mt;
};




template <typename T>
MYSQL_BIND StepInputBind(const T& value);

template <typename T>
struct MysqlType {
  static constexpr enum_field_types value = MYSQL_TYPE_NULL;
  static constexpr bool is_unsigned = false;
};

#define DEFINE_MYSQL_TYPE_SPECIALIZATION(c_type, mysql_type, is_unsigned_type) \
  template <>                                                                  \
  struct MysqlType<c_type> {                                                   \
    static constexpr enum_field_types value = mysql_type;                      \
    static constexpr bool is_unsigned = is_unsigned_type;                      \
  };

DEFINE_MYSQL_TYPE_SPECIALIZATION(int8_t, MYSQL_TYPE_TINY, false)
DEFINE_MYSQL_TYPE_SPECIALIZATION(uint8_t, MYSQL_TYPE_TINY, true)
DEFINE_MYSQL_TYPE_SPECIALIZATION(int16_t, MYSQL_TYPE_SHORT, false)
DEFINE_MYSQL_TYPE_SPECIALIZATION(uint16_t, MYSQL_TYPE_SHORT, true)
DEFINE_MYSQL_TYPE_SPECIALIZATION(int32_t, MYSQL_TYPE_LONG, false)
DEFINE_MYSQL_TYPE_SPECIALIZATION(uint32_t, MYSQL_TYPE_LONG, true)
DEFINE_MYSQL_TYPE_SPECIALIZATION(int64_t, MYSQL_TYPE_LONGLONG, false)
DEFINE_MYSQL_TYPE_SPECIALIZATION(uint64_t, MYSQL_TYPE_LONGLONG, true)
DEFINE_MYSQL_TYPE_SPECIALIZATION(float, MYSQL_TYPE_FLOAT, false)
DEFINE_MYSQL_TYPE_SPECIALIZATION(double, MYSQL_TYPE_DOUBLE, false)
DEFINE_MYSQL_TYPE_SPECIALIZATION(mysql_time, MYSQL_TYPE_DATETIME, true)


// ***********
// Input Bind
// ***********


template <typename T>
inline void StepInputBind(MYSQL_BIND& bind, const T& value) {
  std::memset(&bind, 0, sizeof(bind));
  bind.buffer_type = MysqlType<T>::value;
  bind.buffer = BIND_POINTER_CAST(&value);
  bind.is_unsigned = MysqlType<T>::is_unsigned;
  bind.is_null = 0;
}

// template <>
// inline MYSQL_BIND StepInputBind(const int& value) {
//     MYSQL_BIND bind;
//     bind.buffer_type = MYSQL_TYPE_LONG;
//     bind.buffer = BIND_POINTER_CAST(&value);
//     bind.is_unsigned = 0;
//     bind.is_null = 0;
//     return bind;
// }

// template <>
// inline MYSQL_BIND StepInputBind(const double& value) {
//     MYSQL_BIND bind;
//     bind.buffer_type = MYSQL_TYPE_DOUBLE;
//     bind.buffer = BIND_POINTER_CAST(&value);
//     bind.is_unsigned = 0;
//     bind.is_null = 0;
//     return bind;
// }

template <>
inline void StepInputBind(MYSQL_BIND& bind, const std::string& value) {
  std::memset(&bind, 0, sizeof(bind));
  bind.buffer_type = MYSQL_TYPE_STRING;
  bind.buffer = const_cast<char*>(value.c_str());
  bind.buffer_length = value.length();
  bind.length = &bind.buffer_length;
  bind.is_unsigned = 0;
  bind.is_null = 0;
}

template <>
inline void StepInputBind(MYSQL_BIND& bind, const char* const& value) {
  std::memset(&bind, 0, sizeof(bind));
  bind.buffer_type = MYSQL_TYPE_STRING;
  bind.buffer = BIND_POINTER_CAST(value);
  bind.buffer_length = std::strlen(value);
  bind.length = &bind.buffer_length;
  bind.is_unsigned = 0;
  bind.is_null = 0;
}

inline void StepInputBind(MYSQL_BIND& bind, const char* value) {
  std::memset(&bind, 0, sizeof(bind));
  bind.buffer_type = MYSQL_TYPE_STRING;
  bind.buffer = BIND_POINTER_CAST(value);
  bind.buffer_length = std::strlen(value);
  bind.length = &bind.buffer_length;
  bind.is_unsigned = 0;
  bind.is_null = 0;
}


template <typename... InputArgs>
inline void BindInputImpl(std::vector<MYSQL_BIND>& binds, const InputArgs&... args) {
  binds.resize(sizeof...(InputArgs));
  int i = 0;
  (StepInputBind(binds[i++], args), ...);
}




// ***********
// Output Bind
// ***********


// template <typename T>
// void StepOutputBind(MYSQL_BIND& bind, std::vector<std::byte>& buffer);



template <typename T>
inline void StepOutputBind(MYSQL_BIND& bind, std::vector<std::byte>& buffer, uint8_t& null_flag) {
  bind.buffer_type = MysqlType<T>::value;
  bind.is_unsigned = MysqlType<T>::is_unsigned;
  buffer.resize(sizeof(T));
  bind.buffer = buffer.data();
  bind.is_null = reinterpret_cast<bool*>(&null_flag);
}

template <>
inline void StepOutputBind<std::string>(MYSQL_BIND& bind, std::vector<std::byte>& buffer, uint8_t& null_flag) {
  bind.buffer_type = MYSQL_TYPE_STRING;
  if (buffer.empty()) {
    buffer.resize(32);
  }
  bind.buffer = buffer.data();
  bind.is_null = reinterpret_cast<bool*>(&null_flag);
  bind.buffer_length = buffer.size();
}



template <typename... OutputArgs>
inline void BindOutputImpl(std::vector<MYSQL_BIND>& output_binds, 
                    std::vector<std::vector<std::byte>>& output_buffers,
                    std::vector<uint8_t>& null_flag_buffer) {

  size_t i = 0;
  ((StepOutputBind<OutputArgs>(output_binds[i], output_buffers[i], null_flag_buffer[i]), i++), ...);
}



// ****************
// Result Tuple Set
// ****************
template <typename T>
inline void StepTupleSet(T& value, const MYSQL_BIND& bind) {
  value = *static_cast<const decltype(&value)>(bind.buffer);
}

template <>
inline void StepTupleSet(std::string& value, const MYSQL_BIND& bind) {
  if((*bind.is_null) == 0)
    value.assign(static_cast<const char*>(bind.buffer), *(bind.length));
}

template <typename... OutputArgs>
inline void SetResultTuple(std::tuple<OutputArgs...>& result, 
                          const std::vector<MYSQL_BIND>& output_binds) {

  std::apply([&output_binds](auto&... args) {
      size_t i = 0;
      ((StepTupleSet(args, output_binds[i++])) , ...);
  }, result);
}

}  // namespace trpc::mysql
