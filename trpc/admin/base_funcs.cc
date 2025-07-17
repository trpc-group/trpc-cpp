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

#include "trpc/admin/base_funcs.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <ctime>
#include <map>
#include <utility>
#include <vector>

#include "trpc/util/log/logging.h"

namespace trpc::admin {

inline int64_t GetCurrentTimeUs() {
  timeval now;
  gettimeofday(&now, nullptr);
  return now.tv_sec * 1000000L + now.tv_usec;
}

namespace {
std::size_t max_prof_num = 8;
int max_fd_scan_num = 10003;
std::mutex mutex;
int64_t proc_start_time = GetCurrentTimeUs();
char base_profile_dir[kPathLen] = "./profiling/";
char pprof_tool_path[kPathLen] = "./profiling/pprof.pl";
char flame_graph_tool_path[kPathLen] = "./profiling/flamegraph.pl";
std::map<std::string, WebDisplayType> dtype_map_{{kDisplayDot, WebDisplayType::kDot},
                                                 {kDisplayFlameGraph, WebDisplayType::kFlameGraph},
                                                 {kDisplayText, WebDisplayType::kText}};
}  // namespace

void SetBaseProfileDir(const std::string& path) {
  int tmp_len = path.size() + 1;
  memset(base_profile_dir, 0, kPathLen);
  snprintf(base_profile_dir, tmp_len, "%s", path.c_str());

  memset(pprof_tool_path, 0, kPathLen);
  snprintf(pprof_tool_path, tmp_len + 8, "%spprof.pl", path.c_str());

  memset(flame_graph_tool_path, 0, kPathLen);
  snprintf(flame_graph_tool_path, tmp_len + 13, "%sflamegraph.pl", path.c_str());
}

void SetMaxProfNum(std::size_t num) { max_prof_num = num; }

std::string GetBaseProfileDir() { return base_profile_dir; }

std::string GetPprofToolPath() { return pprof_tool_path; }

std::string GetFlameGraphToolPath() { return flame_graph_tool_path; }

std::size_t GetMaxProfNum() { return max_prof_num; }

int CheckDisplayType(const std::string& display_type) {
  auto it = dtype_map_.find(display_type);
  if (it == dtype_map_.end()) {
    return -1;
  }
  return 0;
}

void GetProfDir(ProfilingType type, std::string* prof_dir) {
  switch (type) {
    case ProfilingType::kCpu:
      prof_dir->append("cpu");
      break;
    case ProfilingType::kHeap:
      prof_dir->append("heap");
      break;
    case ProfilingType::kContention:
      prof_dir->append("contention");
      break;
    default:
      prof_dir->append("unknown");
      break;
  }
}

int GetFiles(ProfilingType type, std::map<std::string, std::string>* mfiles) {
  struct stat path_stat;
  std::string dir_name = base_profile_dir;
  if (stat(dir_name.c_str(), &path_stat) && mkdir(dir_name.c_str(), 0755) < 0) {
    TRPC_LOG_ERROR("stat and mkdir base_profile_dir error: " << std::to_string(errno));
    return -1;
  }
  GetProfDir(type, &dir_name);
  TRPC_LOG_DEBUG("GetFiles, dir_name: " << dir_name);
  if (stat(dir_name.c_str(), &path_stat) && mkdir(dir_name.c_str(), 0755) < 0) {
    TRPC_LOG_ERROR("stat and mkdir dir: " << dir_name << " error: " << std::to_string(errno));
    return -1;
  }

  DIR* dp = opendir(dir_name.c_str());
  if (dp == nullptr) {
    TRPC_LOG_ERROR("GetFiles, opendir fail");
    return -1;
  }
  struct dirent* dirp;
  struct stat st;
  while ((dirp = readdir(dp)) != nullptr) {
    if (dirp->d_name[0] == '.') {
      continue;
    }
    std::string f_name = dir_name + "/" + dirp->d_name;
    stat(f_name.c_str(), &st);
    if (S_ISREG(st.st_mode)) {
      mfiles->insert(std::make_pair(f_name, dirp->d_name));
    }
  }
  closedir(dp);
  return 0;
}

int RemoveDirectory(const std::string& dir_name) {
  DIR* dp = opendir(dir_name.c_str());
  if (dp == nullptr) {
    TRPC_LOG_ERROR("RemoveDirectory, opendir fail");
    return -1;
  }
  struct dirent* dirp;
  struct stat st;
  while ((dirp = readdir(dp)) != nullptr) {
    if (dirp->d_name[0] == '.') {
      continue;
    }
    std::string f_name = dir_name + "/" + dirp->d_name;
    stat(f_name.c_str(), &st);
    if (S_ISREG(st.st_mode)) {
      if (remove(f_name.c_str()) < 0) {
        TRPC_LOG_ERROR("In RemoveDirectory, remove file: " << f_name << " fail");
      }
    }
  }
  if (remove(dir_name.c_str()) < 0) {
    TRPC_LOG_ERROR("In RemoveDirectory, remove dir: " << dir_name << " fail");
  }
  closedir(dp);
  return 0;
}

int MakeNewProfName(ProfilingType type, char* buf, int buf_len, std::map<std::string, std::string>* mfiles) {
  struct stat path_stat;
  if (mfiles->size() >= max_prof_num) {
    // delete the oldest files
    std::map<std::string, std::string>::iterator iter = mfiles->begin();
    if (stat(iter->first.c_str(), &path_stat) >= 0) {
      if (remove(iter->first.c_str()) < 0) {
        TRPC_LOG_ERROR("In MakeNewProfName, remove file: " << iter->first << " fail");
        return -1;
      }
    }

    std::string str_cache = iter->first + ".cache";
    if (stat(str_cache.c_str(), &path_stat) >= 0) {
      if (RemoveDirectory(str_cache) < 0) {
        TRPC_LOG_ERROR("In MakeNewProfName, remove dir: " << str_cache << " fail");
        return -1;
      }
    }

    mfiles->erase(iter);
  }

  std::string dir_name = base_profile_dir;
  GetProfDir(type, &dir_name);
  int nr = snprintf(buf, buf_len, "%s/", dir_name.c_str());
  if (nr < 0) {
    return -1;
  }
  buf += nr;
  buf_len -= nr;

  time_t rawtime;
  time(&rawtime);
  struct tm* timeinfo = localtime(&rawtime);
  strftime(buf, buf_len, "%Y%m%d.%H%M%S", timeinfo);
  return 0;
}

int MakeProfCacheName(const std::string& display_type, std::string* cache_path) {
  struct stat path_stat;
  if (stat(cache_path->c_str(), &path_stat)) {
    TRPC_LOG_ERROR("stat cache file: " << *cache_path << " not exists, error: " << std::to_string(errno));
    return -1;
  }

  cache_path->append(".cache");
  if (stat(cache_path->c_str(), &path_stat) && mkdir(cache_path->c_str(), 0755) < 0) {
    TRPC_LOG_ERROR("stat and mkdir cache_dir: " << *cache_path << " error: " << std::to_string(errno));
    return -1;
  }

  if (strncmp(display_type.c_str(), kDisplayDot, 3) == 0) {
    cache_path->append("/dot");
  } else if (strncmp(display_type.c_str(), kDisplayFlameGraph, 5) == 0) {
    cache_path->append("/flame");
  } else if (strncmp(display_type.c_str(), kDisplayText, 4) == 0) {
    cache_path->append("/text");
  } else {
    TRPC_LOG_ERROR("display_type: " << display_type << " not support");
    return -1;
  }
  return 0;
}

int GetProfileFileContent(const std::string& file_path, std::string* output) {
  struct stat path_stat;
  if (stat(file_path.c_str(), &path_stat)) {
    TRPC_LOG_ERROR("stat fail: " << file_path << " error: " << std::to_string(errno));
    return -1;
  }
  FILE* fp = fopen(file_path.c_str(), "r");
  if (fp != nullptr) {
    char buffer[1024];
    while (1) {
      size_t nr = fread(buffer, 1, sizeof(buffer), fp);
      if (nr != 0) {
        output->append(buffer, nr);
      }
      if (nr != sizeof(buffer)) {
        if (feof(fp)) {
          break;
        } else if (ferror(fp)) {
          TRPC_LOG_ERROR("Encountered error while reading for " << file_path);
          fclose(fp);
          return -1;
        }
        // retry;
      }
    }
    fclose(fp);
    return 0;
  }
  return -1;
}

int WritePprofPerlFile(const std::string& content) {
  struct stat path_stat;
  if (stat(pprof_tool_path, &path_stat)) {
    FILE* fp = fopen(pprof_tool_path, "w");
    if (nullptr == fp) {
      TRPC_LOG_ERROR("In WritePprofPerlFile, fopen fail, errno: " << errno);
      return -1;
    }
    if (fwrite(content.data(), content.size(), 1UL, fp) != 1UL) {
      TRPC_LOG_ERROR("In WritePprofPerlFile, fwrite fail, errno: " << errno);
      fclose(fp);
      return -1;
    }
    fclose(fp);
  }
  return 0;
}

int WriteFlameGraphPerlFile(const std::string& content) {
  struct stat path_stat;
  if (stat(flame_graph_tool_path, &path_stat)) {
    FILE* fp = fopen(flame_graph_tool_path, "w");
    if (nullptr == fp) {
      TRPC_LOG_ERROR("In WriteFlameGraphPerlFile, fopen fail, errno: " << errno);
      return -1;
    }
    if (fwrite(content.data(), content.size(), 1UL, fp) != 1UL) {
      TRPC_LOG_ERROR("In WriteFlameGraphPerlFile, fwrite fail, errno: " << errno);
      fclose(fp);
      return -1;
    }
    fclose(fp);
  }
  return 0;
}

int WriteCacheFile(const std::string& path, const std::string& content) {
  struct stat path_stat;
  if (stat(path.c_str(), &path_stat)) {
    FILE* fp = fopen(path.c_str(), "w");
    if (nullptr == fp) {
      TRPC_LOG_ERROR("In WriteCacheFile, fopen fail, path: " << path << ", errno: " << errno);
      return -1;
    }
    if (fwrite(content.data(), content.size(), 1UL, fp) != 1UL) {
      TRPC_LOG_ERROR("In WriteCacheFile, fwrite fail, path: " << path << ", errno: " << errno);
      fclose(fp);
      return -1;
    }
    fclose(fp);
  }
  return 0;
}

void DisplayTypeToCmd(const std::string& display_type, std::string* cmd) {
  if (display_type == kDisplayDot) {
    cmd->append(" --dot ");
  } else if (display_type == kDisplayFlameGraph) {
    cmd->append(" --collapsed ");
  } else if (display_type == kDisplayText) {
    cmd->append(" --text ");
  }
}

int ExecCommandByPopen(const char* cmd, std::string* output) {
  FILE* pipe = popen(cmd, "r");
  if (pipe == nullptr) {
    TRPC_LOG_ERROR("ExecCommandByPopen, do popen fail");
    return -1;
  }
  char buffer[1024];
  for (;;) {
    size_t nr = fread(buffer, 1, sizeof(buffer), pipe);
    if (nr != 0) {
      output->append(buffer, nr);
    }
    if (nr != sizeof(buffer)) {
      if (feof(pipe)) {
        break;
      } else if (ferror(pipe)) {
        TRPC_LOG_ERROR("Encountered error while reading for the pipe");
        pclose(pipe);
        return -1;
      }
      // retry;
    }
  }
  pclose(pipe);
  return 0;
}

int WriteSysvarsData(const std::string& path, const std::string& content) {
  struct stat path_stat;
  std::string root_dir = base_profile_dir;
  if (stat(root_dir.c_str(), &path_stat) && mkdir(root_dir.c_str(), 0755) < 0) {
    TRPC_LOG_ERROR("stat and mkdir cache_dir: " << root_dir << " error: " << std::to_string(errno));
    return -1;
  }
  std::lock_guard<std::mutex> lock(mutex);
  FILE* fp = fopen(path.c_str(), "w");
  if (nullptr == fp) {
    TRPC_LOG_ERROR("In WriteCacheFile, fopen fail, path: " << path << ", errno: " << errno);
    return -1;
  }

  if (fwrite(content.data(), content.size(), 1UL, fp) != 1UL) {
    TRPC_LOG_ERROR("In WriteCacheFile, fwrite fail, path: " << path << ", errno: " << errno);
    fclose(fp);
    return -1;
  }
  fclose(fp);
  return 0;
}

int ReadSysvarsData(const std::string& path, std::string* output) {
  FILE* fp = fopen(path.c_str(), "r");
  std::lock_guard<std::mutex> lock(mutex);
  if (fp != nullptr) {
    char buffer[1024];
    while (1) {
      size_t nr = fread(buffer, 1, sizeof(buffer), fp);
      if (nr != 0) {
        output->append(buffer, nr);
      }
      if (nr != sizeof(buffer)) {
        if (feof(fp)) {
          break;
        } else if (ferror(fp)) {
          TRPC_LOG_ERROR("Encountered error while reading for " << path);
          fclose(fp);
          return -1;
        }
        // retry;
      }
    }
    fclose(fp);
    return 0;
  }
  return -1;
}

//////////////////////////////////////////////////////////////////
int GetCurrProgressName(std::string* prog_name) {
  char strProcessPath[1024] = {0};
  if (readlink("/proc/self/exe", strProcessPath, 1024) <= 0) {
    TRPC_LOG_ERROR("read process path fail, errno:" << errno);
    return -1;
  }
  char* strProcessName = strrchr(strProcessPath, '/');
  if (!strProcessName) {
    TRPC_LOG_ERROR("get process name fail");
    return -1;
  } else {
    *prog_name = ++strProcessName;
  }
  return 0;
}

void GetGccVersion(std::string* version) {
  version->append(std::to_string(__GNUC__) + ".");
  version->append(std::to_string(__GNUC_MINOR__) + ".");
  version->append(std::to_string(__GNUC_PATCHLEVEL__));
}

struct ReadKernelVersion {
  std::string content;

  static ReadKernelVersion* GetInstance() {
    static ReadKernelVersion instance;
    return &instance;
  }

 private:
  ReadKernelVersion() {
    if (ExecCommandByPopen("uname -ap", &content) != 0) {
      content = "-";
    }
  }
  ReadKernelVersion(const ReadKernelVersion&) = delete;
  ReadKernelVersion& operator=(const ReadKernelVersion&) = delete;
};

void GetKernelVersion(std::string* version) { *version = ReadKernelVersion::GetInstance()->content; }

void GetSystemCoreNums(std::string* str) {
  const int core_nums = sysconf(_SC_NPROCESSORS_ONLN);
  str->append(std::to_string(core_nums));
}

int GetSystemPageSize() {
  int page_size = sysconf(_SC_PAGESIZE);
  return page_size;
}

void GetUserName(std::string* str) {
  // char buf[32];
  // if (getlogin_r(buf, sizeof(buf)) == 0) {
  //     buf[sizeof(buf)-1] = '\0';
  //     str->append(buf);
  // } else {
  //    str->append("unknown");
  // }

  // FIXME(getlogin_r may crash)
  str->append("unknown");
}

void GetCurrentDirectory(std::string* str) {
  char system_buffer[1024] = "";
  if (!getcwd(system_buffer, sizeof(system_buffer))) {
    str->append("unknown");
  } else {
    str->append(system_buffer);
  }
}

int ReadLoadUsage(LoadAverage* load) {
  FILE* fp = fopen("/proc/loadavg", "r");
  if (nullptr == fp) {
    TRPC_LOG_ERROR("Fail to open /proc/loadavg");
    return -1;
  }
  errno = 0;
  if (fscanf(fp, "%lf %lf %lf", &load->loadavg_1m, &load->loadavg_5m, &load->loadavg_15m) != 3) {
    TRPC_LOG_ERROR("Fail to fscanf");
    fclose(fp);
    return -1;
  }
  fclose(fp);
  return 0;
}

//////////////////////////////////////////////////////////// read proc info
struct timeval GetProcUptime() {
  int64_t uptime_us = GetCurrentTimeUs() - proc_start_time;
  timeval tm;
  tm.tv_sec = uptime_us / 1000000L;
  tm.tv_usec = uptime_us - tm.tv_sec * 1000000L;
  return tm;
}

int GetProcRusage(rusage* stat) {
  const int rc = getrusage(RUSAGE_SELF, stat);
  if (rc < 0) {
    TRPC_LOG_ERROR("Fail to getrusage");
    return -1;
  }
  return 0;
}

int ReadProcCommandLine(char* buf, size_t len) {
  int fd = open("/proc/self/cmdline", O_RDONLY);
  if (fd < 0) {
    TRPC_LOG_ERROR("Fail to open /proc/self/cmdline, errno: " << errno);
    return -1;
  }
  int nr = read(fd, buf, len);
  if (nr <= 0) {
    TRPC_LOG_ERROR("Fail to read /proc/self/cmdline");
    close(fd);
    return -1;
  }

  if (static_cast<size_t>(nr) != len) {
    for (int i = 0; i < nr; ++i) {
      if (buf[i] == '\0') {
        buf[i] = '\n';
      }
    }
  }
  close(fd);
  return 0;
}

int ReadProcIo(ProcIO* pi) {
  FILE* fp = fopen("/proc/self/io", "r");
  if (nullptr == fp) {
    TRPC_LOG_ERROR("Fail to open /proc/self/io");
    return -1;
  }
  errno = 0;
  if (fscanf(fp, "%*s %lu %*s %lu %*s %lu %*s %lu %*s %lu %*s %lu %*s %lu", &pi->rchar, &pi->wchar, &pi->syscr,
             &pi->syscw, &pi->read_bytes, &pi->write_bytes, &pi->cancelled_write_bytes) != 7) {
    TRPC_LOG_ERROR("ReadProcDiskIo fscanf fail");
    fclose(fp);
    return -1;
  }
  fclose(fp);
  return 0;
}

/*
int ReadDiskStat(DiskStat* ds) {
  FILE* fp = fopen("/proc/diskstats", "r");
  if (nullptr == fp) {
    TRPC_LOG_ERROR("Fail to open /proc/diskstats");
    return -1;
  }
  errno = 0;
  if (fscanf(fp, "%lld %lld %s %lld %lld %lld %lld %lld %lld %lld "
              "%lld %lld %lld %lld",
              &ds->major_number,
              &ds->minor_mumber,
              &ds->device_name,
              &ds->reads_completed,
              &ds->reads_merged,
              &ds->sectors_read,
              &ds->time_spent_reading_ms,
              &ds->writes_completed,
              &ds->writes_merged,
              &ds->sectors_written,
              &ds->time_spent_writing_ms,
              &ds->io_in_progress,
              &ds->time_spent_io_ms,
              &ds->weighted_time_spent_io_ms) != 14) {
    TRPC_LOG_ERROR("ReadDiskStat, Fail to fscanf");
    fclose(fp);
    return -1;
  }
  fclose(fp);
  return 0;
}
*/

int ReadProcState(ProcStat* stat) {
  FILE* fp = fopen("/proc/self/stat", "r");
  if (fp != nullptr) {
    if (fscanf(fp,
               "%d %*s %c "
               "%d %d %d %d %d "
               "%u %lu %lu %lu "
               "%lu %lu %lu %lu %lu "
               "%ld %ld %ld %ld "
               "%lu %lu %d",
               &stat->pid, &stat->state, &stat->ppid, &stat->pgrp, &stat->session, &stat->tty_nr, &stat->tpgid,
               &stat->flags, &stat->minflt, &stat->cminflt, &stat->majflt, &stat->cmajflt, &stat->utime, &stat->stime,
               &stat->cutime, &stat->cstime, &stat->priority, &stat->nice, &stat->num_threads, &stat->it_real_value,
               &stat->start_time, &stat->vsize, &stat->rss) != 23) {
      TRPC_LOG_ERROR("In ReadProcStatus, fscanf fail, errno: " << errno);
      return -1;
    }
    fclose(fp);
  } else {
    return -1;
  }
  return 0;
}

int ReadProcMemory(ProcMemory* mem) {
  errno = 0;
  FILE* fp = fopen("/proc/self/statm", "r");
  if (nullptr == fp) {
    TRPC_LOG_ERROR("Fail to open /proc/self/statm");
    return -1;
  }
  if (fscanf(fp, "%ld %ld %ld %ld %ld %ld %ld", &mem->size, &mem->resident, &mem->share, &mem->trs, &mem->lrs,
             &mem->drs, &mem->dt) != 7) {
    TRPC_LOG_ERROR("Fail to fscanf /proc/self/statm");
    fclose(fp);
    return -1;
  }
  fclose(fp);
  return 0;
}

int GetProcFdCount(int* fd_count) {
  errno = 0;
  int fd = open("/proc/self/fd", O_RDONLY | O_DIRECTORY);
  if (fd < 0) {
    TRPC_LOG_ERROR("GetFdCount, opendir fail");
    return -1;
  }
  unsigned char buf[512] = {0};
  size_t offset = 0;
  size_t size = 0;
  // Have to limit the scaning which consumes a lot of CPU when #fd
  // are huge (100k+)
  int count = 0;
  for (; count <= max_fd_scan_num; ++count) {
    if (size) {
      LinuxDirent* dirent = reinterpret_cast<LinuxDirent*>(buf + offset);
      offset += dirent->d_reclen;
    }

    if (offset != size) {
      continue;
    }

    const int r = syscall(__NR_getdents64, fd, buf, sizeof(buf));
    if (r == 0) {
      break;
    }
    if (r == -1) {
      TRPC_LOG_ERROR("GetProcFdCount, getdents64 returned an error: " << errno);
      break;
    }
    size = r;
    offset = 0;
  }

  *fd_count = count - 3; /* skipped ., .. and the fd in dr*/
  close(fd);
  return 0;
}

double GetCpuUsage() {
  static double pre_time = -UINT16_MAX;
  static double pre_process_value = 0;

  // Status of current process.
  ProcStat proc_stat;
  int stat_ret = ReadProcState(&proc_stat);
  if (stat_ret != 0) {
    TRPC_LOG_ERROR("get process stat failed, update failed");
    return 0;
  }

  // The time unit for utime and stime in proc_stat is the Linux timer interval.
  // The frequency of Linux is obtained from the system
  static int sys_clk_tck = GetSystemClockTick();
  static int clk_tck = sys_clk_tck > 0 ? sys_clk_tck : 100;
  double cur_process_value = static_cast<double>(proc_stat.utime + proc_stat.stime) / clk_tck;

  // Calculates the duration of this time period in seconds.
  double uptime = GetProcUptime().tv_sec;
  double seconds = uptime - pre_time;
  double cpu_usage = ((cur_process_value - pre_process_value) / seconds) * 100;

  // Updates the starting point of the time interval and update the current interval CPU's expenditure time
  // on the process.
  pre_time = uptime;
  pre_process_value = cur_process_value;

  return cpu_usage;
}

int GetSystemClockTick() {
  int clock_tick = sysconf(_SC_CLK_TCK);
  return clock_tick;
}

}  // namespace trpc::admin
