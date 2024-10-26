//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include "trpc/util/jwt.h"

namespace trpc {

bool Jwt::isValid(const std::string& token, std::map<std::string, std::string>& auth_cfg) {
  try {
    auto decoded_token = jwt::decode(token);

    auto verifier = jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{auth_cfg["secret"]})
        .with_issuer(auth_cfg["iss"]);

    verifier.verify(decoded_token);
    return true;
  } catch (const std::exception& e) {
    return false;
  }
}

}
