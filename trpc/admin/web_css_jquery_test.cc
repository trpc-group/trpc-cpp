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

#include "trpc/admin/web_css_jquery.h"

#include <string.h>

#include <map>
#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::testing {

TEST(WebCssJqueryTest, Test) {
  const char* p2 = admin::HtmlHeader();
  EXPECT_GE(strlen(p2), 10);

  const char* p3 = admin::TabsHead();
  EXPECT_GE(strlen(p3), 10);

  const char* p4 = admin::GridTableStyle();
  EXPECT_GE(strlen(p4), 10);

  const char* p5 = admin::PutSysvarsHeading();
  EXPECT_GE(strlen(p5), 10);

  const char* p6 = admin::CommonOnRecvData();
  EXPECT_GE(strlen(p6), 10);

  std::map<std::string, admin::CmdInfo> m_tabs;
  admin::SetTabItems(&m_tabs);
  EXPECT_GE(m_tabs.size(), 1);

  std::string html;
  admin::PrintTabsBody(&html, m_tabs);
  EXPECT_GE(html.size(), 10);

  html.clear();
  std::string vname = "xxx.yyy";
  std::map<std::string, std::string> mfiles;
  admin::PrintViewListInfo(&html, vname, mfiles);
  EXPECT_GE(html.size(), 10);

  html.clear();
  mfiles[vname] = "ttttttt";
  admin::PrintViewListInfo(&html, vname, mfiles);
  size_t pos = html.find(vname);
  EXPECT_GE(pos, 0);

  html.clear();
  admin::PrintDisplayInfo(&html, admin::kDisplayDot);
  EXPECT_GE(html.size(), 10);

  html.clear();
  admin::PrintDisplayInfo(&html, admin::kDisplayFlameGraph);
  EXPECT_GE(html.size(), 10);

  html.clear();
  admin::PrintDisplayInfo(&html, admin::kDisplayText);
  EXPECT_GE(html.size(), 10);
}

}  // namespace trpc::testing
