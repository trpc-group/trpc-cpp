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

#include "trpc/util/log/default/sinks/local_file/file_helper.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

namespace trpc {

std::string SimplifyDirectory(const std::string& path) {
  std::string result = path;
  std::string::size_type pos = 0;

  while ((pos = result.find("//", pos)) != std::string::npos) {
    result.erase(pos, 1);
  }

  pos = 0;
  while ((pos = result.find("/./", pos)) != std::string::npos) {
    result.erase(pos, 2);
  }

  while (result.substr(0, 4) == "/../") {
    result.erase(0, 3);
  }

  if (result == "/.") {
    return result.substr(0, result.size() - 1);
  }

  if (result.size() >= 2 && result.substr(result.size() - 2, 2) == "/.") {
    result.erase(result.size() - 2, 2);
  }

  if (result == "/") {
    return result;
  }

  if (result.size() >= 1 && result[result.size() - 1] == '/') {
    result.erase(result.size() - 1);
  }

  if (result == "/..") {
    result = "/";
  }

  return result;
}

bool IsFileExist(const std::string& full_file_name, mode_t file_type) {
  struct stat f_stat;
  if (lstat(full_file_name.c_str(), &f_stat) == -1) {
    return false;
  }

  if (!(f_stat.st_mode & file_type)) {
    return false;
  }

  return true;
}

size_t ScanDir(const std::string& file_path, std::vector<std::string>& match_files, FILESELECT f, int max_size) {
  match_files.clear();

  struct dirent** namelist;
  int n = scandir(file_path.c_str(), &namelist, f, alphasort);
  if (n < 0) {
    return 0;
  } else {
    while (n--) {
      if (max_size > 0 && match_files.size() >= (size_t)max_size) {
        free(namelist[n]);
        break;
      } else {
        match_files.emplace_back(namelist[n]->d_name);
        free(namelist[n]);
      }
    }
    free(namelist);
  }

  return match_files.size();
}

void ListDirectory(const std::string& path, std::vector<std::string>& files, bool is_recursive) {
  std::vector<std::string> tf;
  ScanDir(path, tf, 0, 0);

  for (size_t i = 0; i < tf.size(); i++) {
    if (tf[i] == "." || tf[i] == "..") continue;

    std::string s = path + "/" + tf[i];

    if (IsFileExist(SimplifyDirectory(s), S_IFDIR)) {
      files.emplace_back(SimplifyDirectory(s));
      if (is_recursive) {
        ListDirectory(s, files, is_recursive);
      }
    } else {
      files.emplace_back(SimplifyDirectory(s));
    }
  }
}

std::string ExtractFileName(const std::string& full_file_name) {
  if (full_file_name.length() <= 0) {
    return "";
  }

  std::string::size_type pos = full_file_name.rfind('/');
  if (pos == std::string::npos) {
    return full_file_name;
  }

  return full_file_name.substr(pos + 1);
}

std::string ExtractFilePath(const std::string& full_file_name) {
  if (full_file_name.length() <= 0) {
    return "./";
  }

  std::string::size_type pos = 0;

  for (pos = full_file_name.length(); pos != 0; --pos) {
    if (full_file_name[pos - 1] == '/') {
      return full_file_name.substr(0, pos);
    }
  }

  return "./";
}

std::string GetExePath() {
  std::string proc = "/proc/self/exe";
  char buf[2048] = "\0";

  int bufsize = sizeof(buf) / sizeof(char);

  int count = readlink(proc.c_str(), buf, bufsize);

  if (count < 0) {
    std::cerr << "could not get exe path error" << std::endl;
    return "";
  }

  count = (count >= bufsize) ? (bufsize - 1) : count;

  buf[count] = '\0';
  return buf;
}

std::tuple<std::string, std::string> SplitByExtension(const std::string& file_name) {
  auto ext_index = file_name.rfind('.');

  // no valid extension found - return whole path and empty string as
  if (ext_index == std::string::npos || ext_index == 0 || ext_index == file_name.size() - 1) {
    return std::make_tuple(file_name, std::string());
  }

  // treat cases like "/etc/rc.d/somelogfile or "/abc/.hiddenfile"
  auto folder_index = file_name.rfind(kFolderSep);
  if (folder_index != std::string::npos && folder_index >= ext_index - 1) {
    return std::make_tuple(file_name, std::string());
  }

  // finally - return a valid base and extension tuple
  return std::make_tuple(file_name.substr(0, ext_index), file_name.substr(ext_index));
}

}  // namespace trpc
