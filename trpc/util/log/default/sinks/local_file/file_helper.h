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

#include <dirent.h>

#include <functional>
#include <string>
#include <tuple>
#include <vector>

namespace trpc {

// note: not support WIN
const char kFolderSep{'/'};

/// @brief Determine whether to select while traversing files.
/// @return 1-Select, 0-Do not select.
typedef int (*FILESELECT)(const dirent*);

/// @brief Normalize directory names by removing unnecessary elements such as "./".
/// @param path Directory name.
/// @return Normalized directory name.
std::string SimplifyDirectory(const std::string& path);

/// @brief Check if the file exists at the given path.
/// @param full_file_name Full file path.
/// @param file_type File type, default S_IFREG.
/// @return true - exists, false - does not exist.
/// @note If the file is a symbolic link, then determine based on the file pointed to by the symbolic link.
bool IsFileExist(const std::string& full_file_name, mode_t file_type);

/// @brief Scan a directory.
/// @param [in] file_path Path to be scanned.
/// @param [out] match_files Vector table of returned file names.
/// @param [in] f Matching function. If it is NULL, it means all files will be obtained.
/// @param [in] max_size Maximum number of files. If max_size <= 0, return all matching files.
/// @return Number of files.
size_t ScanDir(const std::string& file_path, std::vector<std::string>& match_files, FILESELECT f, int max_size);

/// @brief Traverse a directory to obtain all files and subdirectories under it.
/// @param path         Path to be scanned.
/// @param files        All files under the target path.
/// @param is_recursive Whether to recursively search subdirectories.
void ListDirectory(const std::string& path, std::vector<std::string>& files, bool is_recursive);

/// @brief Extract the path of a file from a complete file name.
///        for example:
///        1、"/usr/local/temp.gif" to "/usr/local/"
///        2、"temp.gif" to  "./"
/// @param full_file_name Complete file name of a file.
/// @return Extracted file path.
std::string ExtractFilePath(const std::string& full_file_name);

/// @brief Extract file name.
///        for example: /usr/local/temp.gif and get temp.gif.
/// @param full_file_name  Complete file name of a file.
/// @return Extracted file name.
std::string ExtractFileName(const std::string& full_file_name);

/// @brief Get the current executable file path.
/// @return Full path name of the executable file.
std::string GetExePath();

/// @brief Get the prefix of the file name.
/// for example:
/// "mylog" => ("mylog", "")"
/// "/data/log/mylog" => ("/data/log/mylog", "")
/// "mylog." => ("mylog.", "")
/// "/dir1/dir2/mylog.txt" => ("/dir1/dir2/mylog", ".txt")
///
/// the starting dot in filenames is ignored (hidden files):
///
/// ".mylog" => (".mylog". "")
/// "my_folder/.mylog" => ("my_folder/.mylog", "")
/// "my_folder/.mylog.txt" => ("my_folder/.mylog", ".txt")
/// @return string return file path and its extension:
std::tuple<std::string, std::string> SplitByExtension(const std::string& file_name);

}  // namespace trpc
