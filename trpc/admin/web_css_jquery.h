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

#include <map>
#include <string>

#include "trpc/admin/base_funcs.h"
#include "trpc/admin/js/flot_min_js.h"
#include "trpc/admin/js/jquery_min_js.h"
#include "trpc/admin/js/viz_min_js.h"
#include "trpc/admin/perl_tools/pprof.h"

namespace trpc::admin {

struct CmdInfo {
  std::string path;
  std::string desc;
};

const char* HtmlHeader();

const char* TabsHead();

const char* GridTableStyle();

const char* PutSysvarsHeading();

void SetTabItems(std::map<std::string, CmdInfo>* m_tabs);

void PrintTabsBody(std::string* html, const std::map<std::string, CmdInfo>& m_tabs);

void PrintDisplayInfo(std::string* html, const std::string& dtype);

void PrintViewListInfo(std::string* html, const std::string& vname, const std::map<std::string, std::string>& mfiles);

const char* CommonOnRecvData();

}  // namespace trpc::admin
