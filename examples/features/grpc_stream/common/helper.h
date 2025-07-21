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

// This code implementation is sourced and modified from
// :https://github.com/grpc/grpc/blob/master/examples/cpp/route_guide/helper.h/cc

#pragma once

#include <string>
#include <vector>

#include "examples/features/grpc_stream/server/stream.trpc.pb.h"

namespace routeguide {

std::string GetDbFileContent(const std::string& db_path);

void ParseDb(const std::string& db, std::vector<Feature>* feature_list);

}  // namespace routeguide
