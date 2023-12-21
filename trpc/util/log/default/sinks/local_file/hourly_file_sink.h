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

#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>
#include <utility>

#include "spdlog/common.h"
#include "spdlog/details/circular_q.h"
#include "spdlog/details/file_helper.h"
#include "spdlog/details/null_mutex.h"
#include "spdlog/details/os.h"
#include "spdlog/details/synchronous_factory.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/sinks/base_sink.h"

namespace trpc {

/// @brief Generator of hourly log file names in format basename.YYYY-MM-DD-HH.ext
struct HourlyFilenameCalculator {
  // Create filename for the form basename.YYYY-MM-DD-HH
  static spdlog::filename_t CalcFileName(const spdlog::filename_t& filename, const std::tm& now_tm) {
    spdlog::filename_t basename, ext;
    std::tie(basename, ext) = spdlog::details::file_helper::split_by_extension(filename);
    return fmt::format(SPDLOG_FILENAME_T("{}_{:04d}-{:02d}-{:02d}-{:02d}{}"), basename, now_tm.tm_year + 1900,
                       now_tm.tm_mon + 1, now_tm.tm_mday, now_tm.tm_hour, ext);
  }
};

/// @brief Rotating file sink based on hour.
///        If truncate != false , the created file will be truncated.
///        If max_files > 0, retain only the last max_files and delete previous.
template <typename Mutex, typename FileNameCalc = HourlyFilenameCalculator>
class HourlyFileSink : public spdlog::sinks::base_sink<Mutex> {
 public:
  /// @brief create hourly file sink
  HourlyFileSink(spdlog::filename_t base_filename, bool truncate = false, uint16_t max_files = 0,
                 uint16_t hour_interval = 1)
      : base_filename_(std::move(base_filename)),
        truncate_(truncate),
        max_files_(max_files),
        hour_interval_(hour_interval),
        filenames_q_() {
    if (hour_interval_ > 23) {
      std::cerr << "HourlyFileSink, Invalid hour_interval_: " << hour_interval_ << std::endl;
    }
    auto now = spdlog::log_clock::now();
    auto filename = FileNameCalc::CalcFileName(base_filename_, NowTime(now));
    file_helper_.open(filename, truncate_);
    rotation_tp_ = NextRotationTimePoint();

    if (max_files_ > 0) {
      filenames_q_ = spdlog::details::circular_q<spdlog::filename_t>(static_cast<size_t>(max_files_));
      filenames_q_.push_back(std::move(filename));
    }
  }

 protected:
  /// @brief sink_it_ is inherited from spdlog, which is used to write log.
  void sink_it_(const spdlog::details::log_msg& msg) override {
    auto time = msg.time;
    bool should_rotate = time >= rotation_tp_;
    if (should_rotate) {
      auto filename = FileNameCalc::CalcFileName(base_filename_, NowTime(time));
      file_helper_.open(filename, truncate_);
      rotation_tp_ = NextRotationTimePoint();
    }

    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    file_helper_.write(formatted);
    // Do the cleaning only at the end because it might throw on failure.
    if (should_rotate && max_files_ > 0) {
      DeleteOldFile();
    }
  }

  void flush_() override { file_helper_.flush(); }

  /// @brief Gets the size of the file
  size_t GetFileSize() { return file_helper_.size(); }

  /// @brief Delete the file N rotations ago.
  /// @note Throw spdlog_ex on failure to delete the old file.
  void DeleteOldFile() {
    using spdlog::details::os::filename_to_str;
    using spdlog::details::os::remove_if_exists;

    spdlog::filename_t current_file = FileName();
    if (filenames_q_.full()) {
      auto old_filename = std::move(filenames_q_.front());
      filenames_q_.pop_front();
      bool ok = remove_if_exists(old_filename) == 0;
      if (!ok) {
        filenames_q_.push_back(std::move(current_file));
        SPDLOG_THROW(spdlog::spdlog_ex("Failed removing hourly file " + filename_to_str(old_filename), errno));
      }
    }
    filenames_q_.push_back(std::move(current_file));
  }

  /// @brief Gets the size of the log file handle queue
  size_t GetFileNamesQueueSize() { return filenames_q_.size(); }

  /// @brief Calculate the next printing time
  spdlog::log_clock::time_point NextRotationTimePoint() {
    auto now = spdlog::log_clock::now();
    tm date = NowTime(now);
    date.tm_min = 0;
    date.tm_sec = 0;
    auto rotation_time = spdlog::log_clock::from_time_t(std::mktime(&date));
    return {rotation_time + std::chrono::hours(GetHourInterval())};
  }

  /// @brief Gets the interval for the printing time
  inline uint16_t GetHourInterval() { return hour_interval_; }

  /// @brief Returns the current moment
  tm NowTime(const spdlog::log_clock::time_point& tp) {
    time_t tnow = spdlog::log_clock::to_time_t(tp);
    return spdlog::details::os::localtime(tnow);
  }

  /// @brief Getting the filename
  const spdlog::filename_t& FileName() const { return file_helper_.filename(); }

  // The original filename to print
  spdlog::filename_t base_filename_;

  // The time at which the next print will occur
  spdlog::log_clock::time_point rotation_tp_;

  // The filehelper provided by spdlog for file operations
  spdlog::details::file_helper file_helper_;

  // It is up to spdlog to truncate when printing
  bool truncate_;

  // Maximum number of files
  uint16_t max_files_;

  // Print time interval(h)
  uint16_t hour_interval_;

  // Scroll log file handle queue
  spdlog::details::circular_q<spdlog::filename_t> filenames_q_;
};

using HourlyFileSinkMt = HourlyFileSink<std::mutex>;

}  // namespace trpc
