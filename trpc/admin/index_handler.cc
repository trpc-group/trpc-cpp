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

#include "trpc/admin/index_handler.h"

namespace trpc::admin {

void IndexHandler::PrintAhrefBody(std::string* html, const std::map<std::string, CmdInfo>& m_tabs) {
  for (const auto& it : m_tabs) {
    html->append("<a href='");
    html->append(it.second.path);
    html->append("'>");
    html->append(it.first);
    html->append("</a> ");
    html->append(it.second.desc);
    html->append("<br>\n");
  }
}

void IndexHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                 rapidjson::Document::AllocatorType& alloc) {
  std::map<std::string, CmdInfo> m_tabs;
  SetTabItems(&m_tabs);

  std::string html = HtmlHeader();
  html.append(TabsHead());
  html.append("</head>\n");

  html.append("<body>\n");
  PrintTabsBody(&html, m_tabs);
  html.append("<br>\n<br>\n");

  html.append("<a href=\"https://github.com/trpcgroup/trpc-cpp\">trpc-cpp-github</a>\n");
  html.append("<br>\n<br>\n");

  PrintAhrefBody(&html, m_tabs);
  html.append("\n\n");
  html.append("</body>\n</html>\n");

  result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
}

}  // namespace trpc::admin
