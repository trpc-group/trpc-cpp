//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>  // timeval, gettimeofday
#include <sys/types.h>
#include <time.h>  // timespec, clock_gettime
#include <unistd.h>

#include <cstddef>
#include <map>
#include <mutex>
#include <string>

namespace trpc::admin {

constexpr int kPathLen{512};
constexpr int kTypeLen{10};

constexpr char kDisplayDot[kTypeLen] = "dot";
constexpr char kDisplayFlameGraph[kTypeLen] = "flame";
constexpr char kDisplayText[kTypeLen] = "text";

/// @brief The content type of profile.
enum class ProfilingType {
  kCpu = 0,
  kHeap = 1,
  kContention = 2,
};

/// @brief The content type of web-display.
enum class WebDisplayType {
  kDot = 0,
  kFlameGraph = 1,
  kText = 2,
};

/// @brief Process status.
struct ProcStat {
  int pid;
  char state;
  int ppid;
  int pgrp;
  int session;
  int tty_nr;
  int tpgid;
  unsigned flags;
  uint64_t minflt;
  uint64_t cminflt;
  uint64_t majflt;
  uint64_t cmajflt;
  uint64_t utime;
  uint64_t stime;
  uint64_t cutime;
  uint64_t cstime;
  int64_t priority;
  int64_t nice;
  int64_t num_threads;
  int64_t it_real_value;
  uint64_t start_time;
  uint64_t vsize;
  int rss;
};

/// @brief Disk IO status.
struct ProcIO {
  /// Read bytes
  size_t rchar;

  /// Write bytes
  size_t wchar;

  /// Read bytes by system call invocations.
  size_t syscr;

  /// Write bytes by system call invocations.
  size_t syscw;

  /// Directly read bytes from disk.
  size_t read_bytes;

  /// Directly write bytes to disk.
  size_t write_bytes;

  /// Cancel write bytes.
  size_t cancelled_write_bytes;
};

/// @brief Disk IO statistics.
struct DiskStat {
  int64_t major_number;
  int64_t minor_number;
  char device_name[64];

  /// wMB/s wKB/s
  int64_t reads_completed;
  /// rrqm/s
  int64_t reads_merged;
  /// rsec/s
  int64_t sectors_read;
  int64_t time_spent_reading_ms;
  /// rKB/s rMB/s
  int64_t writes_completed;
  /// wrqm/s
  int64_t writes_merged;
  /// wsec/s
  int64_t sectors_written;
  int64_t time_spent_writing_ms;
  int64_t io_in_progress;
  int64_t time_spent_io_ms;
  int64_t weighted_time_spent_io_ms;
};

/// @brief Memory status.
struct ProcMemory {
  /// total program size
  int64_t size;
  /// resident set size
  int64_t resident;
  /// shared pages
  int64_t share;
  /// text (code)
  int64_t trs;
  /// library
  int64_t lrs;
  /// data/stack
  int64_t drs;
  /// dirty pages
  int64_t dt;
};

/// @brief Load status.
struct LoadAverage {
  double loadavg_1m;
  double loadavg_5m;
  double loadavg_15m;
};

/// @brief process runtime status.
struct ProcRusage {
  time_t real_time;
  time_t sys_time;
  time_t user_time;
};

/// @brief Directory entry.
struct LinuxDirent {
  uint64_t d_ino;
  int64_t d_off;
  int16_t d_reclen;
  unsigned char d_type;
  char d_name[0];
};

/////////////////////////////////////////////////////////////////////////////
void SetBaseProfileDir(const std::string& path);

void SetMaxProfNum(std::size_t num);

std::string GetBaseProfileDir();

std::string GetPprofToolPath();

std::string GetFlameGraphToolPath();

std::size_t GetMaxProfNum();

int CheckDisplayType(const std::string& display_type);

void GetProfDir(ProfilingType type, std::string* prof_dir);

int GetFiles(ProfilingType type, std::map<std::string, std::string>* mdir);

int RemoveDirectory(const std::string& dir_name);

int MakeNewProfName(ProfilingType type, char* buf, int buf_len, std::map<std::string, std::string>* mfiles);

int MakeProfCacheName(const std::string& display_type, std::string* cache_path);

int GetProfileFileContent(const std::string& file_path, std::string* output);

int WritePprofPerlFile(const std::string& content);

int WriteFlameGraphPerlFile(const std::string& content);

int WriteCacheFile(const std::string& path, const std::string& content);

void DisplayTypeToCmd(const std::string& display_type, std::string* cmd);

int ExecCommandByPopen(const char* cmd, std::string* output);

int WriteSysvarsData(const std::string& path, const std::string& content);

int ReadSysvarsData(const std::string& path, std::string* output);

//////////////////////////////////////////////////////////////////////////
int GetCurrProgressName(std::string* prog_name);

void GetGccVersion(std::string* gcc_version);

void GetKernelVersion(std::string* kernel_version);

void GetSystemCoreNums(std::string* core_nums);

void GetUserName(std::string* str);

void GetCurrentDirectory(std::string* str);

int ReadLoadUsage(LoadAverage* load);

int GetSystemPageSize();

////////////////////////////////////////////////////////////////////////// read proc info
int GetProcRusage(rusage* stat);

int ReadProcState(ProcStat* stat);

int ReadProcCommandLine(char* buf, size_t len);

int ReadProcIo(ProcIO* pi);

// int ReadDiskStat(DiskStat* ds);

struct timeval GetProcUptime();

int GetProcFdCount(int* fd_count);

int ReadProcMemory(ProcMemory* mem);

double GetCpuUsage();

int GetSystemClockTick();

}  // namespace trpc::admin
