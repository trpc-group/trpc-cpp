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

#include "trpc/util/log/default/sinks/local_file/local_file_sink.h"

namespace trpc {

/// @brief Initializing the sink
/// @param params Initializing parameters
/// @return int   Return 0 means success and a complex number means failure
int LocalFileSink::Init(const LocalFileSinkConfig& config) {
  config_ = config;
  if (config_.roll_type == "by_size") {
    // spdlog needs a sink with shared_ptr as input
    sink_ = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(config_.filename, config_.roll_size,
                                                                   config_.reserve_count);
  } else if (config_.roll_type == "by_day") {
    sink_ = std::make_shared<spdlog::sinks::daily_file_sink_mt>(config_.filename, config_.rotation_hour,
                                                                config_.rotation_minute, false, config_.reserve_count);
    if (config_.remove_timout_file_switch) {
      RemoveTimeoutDailyLogFile(config_.filename, config_.reserve_count);
    }
  } else if (config_.roll_type == "by_hour") {
    sink_ =
        std::make_shared<trpc::HourlyFileSinkMt>(config_.filename, false, config_.reserve_count, config_.hour_interval);
  } else {
    std::cerr << "local_file_sink roll_type is invalid: " << config_.roll_type << std::endl;
  }

  std::string format = config_.format;
  std::string eol = config_.eol ? spdlog::details::os::default_eol : "";

  auto formatter = format.empty()
                       ? std::make_unique<spdlog::pattern_formatter>(spdlog::pattern_time_type::local, eol)
                       : std::make_unique<spdlog::pattern_formatter>(format, spdlog::pattern_time_type::local, eol);
  sink_->set_formatter(std::move(formatter));

  return 0;
}

bool LocalFileSink::RemoveTimeoutDailyLogFile(const std::string& file_name, int reserver_count) {
  std::string file_exe_path = trpc::ExtractFilePath(file_name);
  std::vector<std::string> files;
  // Find all filenames in the current path
  // Each element in files is a filename with a path. For example:./test_log_2021-04-22.log
  trpc::ListDirectory(file_exe_path, files, false);

  std::string basename{""}, ext{""};
  std::tie(basename, ext) = trpc::SplitByExtension(file_name);
  std::unordered_map<std::string, tm> target_files;
  std::string::size_type pos = 0;
  bool ret{true};
  for (auto& item : files) {
    pos = item.find(basename);
    if (pos == std::string::npos) continue;
    // Split time in tm_time
    tm tm_time{0};
    ret = SplitFileNameTime(item, ext, tm_time);
    if (!ret) continue;
    target_files.emplace(item, tm_time);
  }

  for (auto& file : target_files) {
    std::time_t now = time(NULL);
    tm* now_tm = localtime(&now);

    // timeout's filename identifier
    bool timeout_flag = (now_tm->tm_year != file.second.tm_year) || (now_tm->tm_mon != file.second.tm_mon) ||
                                (now_tm->tm_mday - file.second.tm_mday + 1 > reserver_count)
                            ? true
                            : false;
    if (timeout_flag) {
      int ret = spdlog::details::os::remove_if_exists(file.first);
      if (ret == 0) {
        std::cout << "delete timeout daily log succ, filename:" << file.first << std::endl;
      }
    }
  }

  return true;
}

bool LocalFileSink::SplitFileNameTime(const std::string& file_name, const std::string& ext, tm& tm_time) {
  std::string::size_type start_pos, end_pos;
  end_pos = file_name.rfind(ext);
  if (end_pos == std::string::npos) {
    end_pos = file_name.length();
  }

  start_pos = file_name.rfind('_');
  if (start_pos == std::string::npos) {
    return false;
  }

  std::string day_info = file_name.substr(start_pos + 1, end_pos - start_pos - 1);

  // date regex based on https://www.regular-expressions.info/dates.html
  // The log exactly match with suffix log files, based on the information of
  // day format {} _ {: 4 d} - {: 02 d} - {: 02 d} {}, for example trpc_2021-04-23.
  std::regex reg(R"((19|20)\d\d-(0[1-9]|1[012])-(0[1-9]|[12][0-9]|3[01])$)");
  std::smatch match;
  bool is_match = std::regex_match(day_info, match, reg);
  if (!is_match) {
    return false;
  }

  char buf[128] = {0};
  snprintf(buf, sizeof(buf), "%s", day_info.c_str());
  // Convert string to tm time tm_mday
  strptime(buf, "%Y-%m-%d", &tm_time);
  return true;
}

}  // namespace trpc
