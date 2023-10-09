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

#include <any>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fmt/core.h"
#include "fmt/format.h"
#include "fmt/printf.h"

#include "trpc/util/ref_ptr.h"

#ifndef TRPC_NOLOG_LEVEL
#define TRPC_NOLOG_LEVEL Level::info
#endif

namespace trpc {

namespace detail {

/// @private For internal use purpose only.
template <typename... Ts>
auto MakeSubpackTuple(Ts&&... xs) {
  return std::tuple<Ts...>(std::forward<Ts>(xs)...);
}

}  // namespace detail

/// @brief Logging plugin base class, support multiple instances, log levels and log attributes
class Log : public RefCounted<Log> {
 public:
  enum Level {
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    error = 4,
    critical = 5,
  };

  Log() = default;

  virtual ~Log() = default;

  /// @brief Initialization
  virtual int Init(const std::any& params) { return 0; }

  /// @brief When creating threads within the implementation of the sink component,
  //         this interface can be used uniformly.
  virtual void Start() {}

  /// @brief When creating threads within the implementation of the sink component,
  //         it is necessary to implement the interface for stopping the thread.
  virtual void Stop() {}

  /// @brief Destroying various resources specific to a component.
  /// @return int           The returned status code, with 0 indicating normal
  virtual int Destroy() { return 0; }

  /// @brief  Determine whether the log level of the current instance meets the requirements for printing this log.
  /// @param  instance_name Log instance name
  /// @param  level         Log instance level
  /// @return true/false
  /// @private For internal use purpose only.
  virtual bool ShouldLog(const char* instance_name, Level level) const = 0;

  /// @brief  Output log to a sink instance.
  /// @param  instance_name Log instance name
  /// @param  level         Log instance level
  /// @param  filename_in   which source file this log comes from
  /// @param  line_in       which line in the source file this log comes from
  /// @param  funcname_in   which function in the source file this log comes from
  /// @param  msg           Log message
  /// @param  filter_data   Log extend message, Can be used for remote plug-ins
  virtual void LogIt(const char* instance_name, Level level, const char* filename_in, int line_in,
                     const char* funcname_in, std::string_view msg,
                     const std::unordered_map<uint32_t, std::any>& filter_data = {}) const = 0;

  /// @brief  Determine if the log level of the current instance meets the requirements for printing this log (console)
  /// @param  instance_name Log instance name
  /// @param  level         Log instance level
  /// @return true/false
  static bool ShouldNoLog(const char* instance_name, Level level) { return level >= TRPC_NOLOG_LEVEL; }

  /// @brief  Used to output relevant error information when printing fails
  ///         Output format: "method name_file name_line number_msg"
  /// @param  instance_name Log instance name
  /// @param  level         Log instance level
  /// @param  filename_in   which source file this log comes from
  /// @param  line_in       which line in the source file this log comes from
  /// @param  funcname_in   which function in the source file this log comes from
  /// @param  msg           Log message
  /// @param  simple        The default is true, which simply outputs msg, and false,
  /// @private For internal use purpose only.
  static void NoLog(const char* instance_name, Level level, const char* filename_in, int line_in,
                    const char* funcname_in, const std::string_view& msg, bool simple = true) {
    std::string data =
        simple ? msg.data() : (fmt::format("--- {}@{}:{} --- {}", funcname_in, filename_in, line_in, msg));
    if (level >= Level::error)
      std::cerr << data << std::endl;
    else
      std::cout << data << std::endl;
  }

  /// @brief Gets the priority of the current log instance
  /// @param  instance_name Log instance name
  /// @return std::pair<Level, bool>  Get the log level configured for the instance
  virtual std::pair<Level, bool> GetLevel(const char* instance_name) const = 0;

  /// @brief Sets the priority of the current log instance.
  /// @param  instance_name Log instance name
  /// @param  level         Log instance level
  /// @return std::pair<Level, bool>  Set the log level configured for the instance
  virtual std::pair<Level, bool> SetLevel(const char* instance_name, Level level) = 0;

  /// @brief Returns the print types supported by the fmt library.
  /// @private For internal use purpose only.
  template <typename E, typename = typename std::enable_if<std::is_enum_v<E>>::type>
  [
      [deprecated("Enum class types do not allow any implicit conversions. "
                  "If you must convert, press static_cast to force the conversion only.")]] static constexpr auto
  TypeConvert(E e) {
    return static_cast<std::underlying_type_t<E>>(e);
  }
  template <typename E, typename = typename std::enable_if<!std::is_enum_v<E>>::type>
  static constexpr const E& TypeConvert(const E& e) {
    return e;
  }

  /// @brief Batch conversion parameter package types.
  /// @private For internal use purpose only.
  template <typename... Ts>
  static constexpr auto TypeConvertAsTuple(Ts&&... args) {
    return detail::MakeSubpackTuple(TypeConvert(args)...);
  }

  /// @brief Some variables passed by the business are of
  /// type fmt::format that are not supported and require an identification conversion.
  /// @private For internal use purpose only.
  template <typename S, typename... Ts>
  static auto LogFormat(const S& format, Ts&&... args) {
    return std::apply([&](const auto&... args) { return fmt::format(fmt::runtime(format), args...); },
                      TypeConvertAsTuple(args...));
  }

  /// @brief Some of the variables passed by the business are not of the type required by
  ///        fmt::sprintf and require a type conversion.
  /// @private For internal use purpose only.
  template <typename S, typename... Ts>
  static auto LogSprintf(const S& format, Ts&&... args) {
    return std::apply([&](const auto&... args) { return fmt::sprintf(format, args...); }, TypeConvertAsTuple(args...));
  }
};
using LogPtr = RefPtr<Log>;

/// @brief Logging plugin factories: Only the "default" log plugins provided by the framework are allowed to register,
///        also designed as factories to unify the design style of all plugins in the framework.
class LogFactory {
 public:
  /// @brief Getting the instance,Because the log plugin is special,
  /// @note notice: The release order of the static object is uncertain when the program exits,
  /// and the user's static function may still use the log factory to obtain the log plugin instance,
  /// so the log factory object is constructed here and is not released.
  /// @return LogFactory*
  static LogFactory* GetInstance() {
    static LogFactory* instance = new LogFactory();
    return instance;
  }

  LogFactory() = default;
  ~LogFactory() = default;
  LogFactory(const LogFactory&) = delete;
  LogFactory& operator=(const LogFactory&) = delete;

  /// @brief Registering plugin instances
  /// @param log Plugin instance pointer
  void Register(const LogPtr& log) { log_ = log; }

  /// @brief Get the log plugin instance
  const LogPtr& Get() const { return log_; }

  /// @brief Release the log plugin instance
  void Reset() { log_.Reset(); }

 private:
  // Returns the "default" log plugin instance
  LogPtr log_;
};

}  // namespace trpc
