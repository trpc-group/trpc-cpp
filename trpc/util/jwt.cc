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

#include "trpc/util/jwt.h"

namespace trpc {

bool Jwt::isValid(const std::string& token, std::map<std::string, std::string>& auth_cfg) {
  try {
    auto decoded_token = jwt::decode(token);

    auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::hs256{auth_cfg["secret"]});

    // Verify if the config exists.
    if (auth_cfg.count("iss")) {
      verifier.with_issuer(auth_cfg["iss"]);
    }
    if (auth_cfg.count("sub")) {
      verifier.with_subject(auth_cfg["sub"]);
    }
    if (auth_cfg.count("aud")) {
      verifier.with_audience(auth_cfg["aud"]);
    }

    verifier.verify(decoded_token);
    return true;
  } catch (const std::exception& e) {
    return false;
  }
}

}  // namespace trpc
