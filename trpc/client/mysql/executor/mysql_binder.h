#pragma once

#include "mysqlclient/mysql.h"
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>
#include <iostream>
#include "trpc/client/mysql/executor/mysql_type.h"

#define BIND_POINTER_CAST(v) (const_cast<void*>(static_cast<const void*>(v)))

namespace trpc::mysql {

///@brief map type information for mysql api
template <typename T>
struct MysqlTypeInfo {
  static constexpr enum_field_types value = MYSQL_TYPE_NULL;
  static constexpr bool is_unsigned = false;
};


#define DEFINE_MYSQL_TYPE_SPECIALIZATION(c_type, mysql_type, is_unsigned_type) \
  template <>                                                                  \
  struct MysqlTypeInfo<c_type> {                                                   \
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
DEFINE_MYSQL_TYPE_SPECIALIZATION(MysqlTime, MYSQL_TYPE_DATETIME, true)



// ***********
// Input Bind
// ***********

template <typename T, typename = std::enable_if_t<!std::is_convertible<T, std::string_view>::value>>
inline void StepInputBind(MYSQL_BIND& bind, const T& value) {
  std::memset(&bind, 0, sizeof(bind));
  bind.buffer_type = MysqlTypeInfo<T>::value;
  bind.buffer = BIND_POINTER_CAST(&value);
  bind.is_unsigned = MysqlTypeInfo<T>::is_unsigned;
}


inline void StepInputBind(MYSQL_BIND& bind, const MysqlBlob& value) {
  std::memset(&bind, 0, sizeof(bind));
  bind.buffer_type = MYSQL_TYPE_BLOB;
  bind.buffer = BIND_POINTER_CAST(value.data_ptr());
  bind.buffer_length = value.size();
  bind.length = &bind.buffer_length;
  bind.is_unsigned = false;
}


inline void StepInputBind(MYSQL_BIND& bind, const MysqlTime& value) {
  std::memset(&bind, 0, sizeof(bind));
  bind.buffer_type = MYSQL_TYPE_DATETIME;
  bind.buffer = BIND_POINTER_CAST(value.DataConstPtr());
  bind.is_unsigned = true;
}

///@brief use string_view to handle "string", "char *, char[N]"
inline void StepInputBind(MYSQL_BIND& bind, std::string_view value) {
  std::memset(&bind, 0, sizeof(bind));
  bind.buffer_type = MYSQL_TYPE_STRING;
  bind.buffer = BIND_POINTER_CAST(value.data());
  bind.buffer_length = value.length();
  bind.length = &bind.buffer_length;
  bind.is_unsigned = false;
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

template <typename T>
inline void StepOutputBind(MYSQL_BIND& bind, std::vector<std::byte>& buffer, uint8_t& null_flag) {
  bind.buffer_type = MysqlTypeInfo<T>::value;
  bind.is_unsigned = MysqlTypeInfo<T>::is_unsigned;
  buffer.resize(sizeof(T));
  bind.buffer = buffer.data();
  bind.is_null = reinterpret_cast<bool*>(&null_flag);
}

template <>
inline void StepOutputBind<std::string>(MYSQL_BIND& bind, std::vector<std::byte>& buffer, uint8_t& null_flag) {
  bind.buffer_type = MYSQL_TYPE_STRING;
  if (buffer.empty()) {
    buffer.resize(32);  // buffer size will usually be set by MysqlExecutor::QueryHandle according to the MysqlResultsOption
  }
  bind.buffer = buffer.data();
  bind.is_null = reinterpret_cast<bool*>(&null_flag);
  bind.buffer_length = buffer.size();
}

template <>
inline void StepOutputBind<MysqlBlob>(MYSQL_BIND& bind, std::vector<std::byte>& buffer, uint8_t& null_flag) {
  bind.buffer_type = MYSQL_TYPE_BLOB;
  if (buffer.empty()) {
    buffer.resize(32); // buffer size will usually be set by MysqlExecutor::QueryHandle according to the MysqlResultsOption
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

inline void StepTupleSet(MysqlTime& value, const MYSQL_BIND& bind) {
  value = {*static_cast<MysqlTime*>(bind.buffer)};
}

inline void StepTupleSet(std::string& value, const MYSQL_BIND& bind) {
  if((*bind.is_null) == 0)
    value.assign(static_cast<const char*>(bind.buffer), *(bind.length));
}

inline void StepTupleSet(MysqlBlob& value, const MYSQL_BIND& bind) {
  if((*bind.is_null) == 0)
    value = MysqlBlob(static_cast<const char*>(bind.buffer), *(bind.length));
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
