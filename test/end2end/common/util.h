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

#include <functional>
#include <map>
#include <memory>

#include "rapidjson/document.h"

#include "trpc/common/future/future.h"
#include "trpc/util/function.h"

namespace trpc::testing {

/// @brief Parse the configs' paths that server and client process need to startup.
/// @note The startup command is assumed to be: ./executable_binary --config=client_config_path
/// --config=server_config_path --xxx=other_flag. `other_flag` will be shared by both progresses by containing this
/// parameter inner client_argv and server_argv.
/// @param[in] argc argc of main function.
/// @param[in] argv argv of main function.
/// @param[out] test_argc how many args of client and server process have.
/// @param[out] client_argv argv of client process.
/// @param[out] server_argv argv of server process.
bool ExtractServerAndClientArgs(int argc, char** argv, int* test_argc, char*** client_argv, char*** server_argv);

/// @brief Client process can load config file from it's argc and argv parameters parsed by
/// `ExtractServerAndClientArgs` to initialize environment.
/// @param argc argc of client process.
/// @param argv argv of client process.
void InitializeRuntimeEnv(int argc, char** argv);

/// @brief Destroy client progress environment initialized by `InitializeRuntimeEnv`.
void DestroyRuntimeEnv();

/// @brief Running task in fiber or thread by determining whether client config file contains fiber nor separte/merge
/// threadmodel.
/// @note This function must be called after `InitializeRuntimeEnv`. It cannot be called after `DestroyRuntimeEnv`.
/// @param fn task logic.
void RunByEnv(Function<void(void)>&& fn);

using PromPtr = std::shared_ptr<Promise<>>;

/// @brief Running task in merge worker thread.
/// @note Must ensure client process config file contains merge threadmodel.
/// @param fn task logic. When the task is done, must SetValue on PromPtr parameter to notify returned Future to
/// continue.
/// @return return Future that can used to block until task done.
Future<> RunAsMerge(Function<void(PromPtr)>&& fn);

/// @brief Check if the json struct contains a specified key-value pair.
/// @param[in] data json struct.
/// @param[in] key key.
/// @param[in] value value.
/// @return true if contains, false otherwise.
bool CheckJsonKV(const ::rapidjson::Document& data, const char* key, const char* value);

/// @brief Build a json-format string with key-value pairs.
/// @param[in] data key-value pairs.
/// @return json-format string.
std::string ConstructJsonStr(const std::map<std::string, std::string>& data);

}  // namespace trpc::testing
