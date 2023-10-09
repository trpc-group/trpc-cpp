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

#include "trpc/common/config/config_helper.h"

#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <cstdlib>

namespace trpc {

bool ConfigHelper::Init(const std::string& conf_path) {
  if (!conf_path_.empty() && conf_path == conf_path_)  // already inited
    return true;

  conf_path_ = conf_path;

  try {
    auto conf = LoadFromPath(conf_path_);
    yaml_parser_.Load(conf);
  } catch (std::exception& ex) {
    std::cerr << "init config error: " << ex.what() << std::endl;
    return false;
  }
  return true;
}

bool ConfigHelper::Reload() {
  YamlParser yaml_parser;
  try {
    auto conf = LoadFromPath(conf_path_);
    yaml_parser.Load(conf);
  } catch (std::exception& ex) {
    std::cerr << "reload config error: " << ex.what() << std::endl;
    return false;
  }

  for (auto&& iter : update_notifiers_) {
    const auto& update_cb = iter.second;
    update_cb(yaml_parser.GetYAML());
  }
  return true;
}

std::string ConfigHelper::LoadFromPath(const std::string& conf_path) {
  std::ifstream ifs(conf_path);
  if (!ifs) {
    throw YAML::BadFile(conf_path);
  }

  // perf. ?
  std::stringstream ss;
  ss << ifs.rdbuf();

  return ExpandEnv(ss.str());
}

std::string ConfigHelper::ExpandEnv(const std::string& str) {
  std::string ret;

  std::regex re(R"(\$\{(\w+)\})");

  std::sregex_iterator ri_beg(str.begin(), str.end(), re);
  std::sregex_iterator ri_end;

  std::ssub_match last;
  last.first = str.begin();
  last.second = str.end();

  for (auto i = ri_beg; i != ri_end; ++i) {
    const std::smatch& m = *i;
    ret.append(m.prefix().first, m.prefix().second);

    auto name = m[1].str();
    // if (name.empty()) continue;  // never empty (\w+)
    char* value = std::getenv(name.data());
    // Whether env is "" or not set, template is rendered to {null}
    if (value) ret.append(value);

    std::cout << "render env template " << m[0].str() << " to " << (value ? value : "{null}")
              << std::endl;

    // Is copy-constructor valid?
    // https://en.cppreference.com/w/cpp/regex/sub_match/sub_match
    last = m.suffix();
  }

  ret.append(last.first, last.second);

  return ret;
}

void ConfigHelper::RegisterConfigUpdateNotifier(const std::string& notify_name,
                                                const std::function<void(const YAML::Node&)>& notify_cb) {
  update_notifiers_[notify_name] = notify_cb;
}

bool ConfigHelper::IsNodeExist(const std::initializer_list<std::string>& areas) const {
  try {
    std::ostringstream path;
    YAML::Node node;
    node.reset(yaml_parser_.GetYAML());
    for (const auto& area : areas) {
      path << area << " ";
      if (node[area]) {
        node.reset(node[area]);
      } else {
        return false;
      }
    }
    return true;
  } catch (std::exception& ex) {
    std::cerr << "get node error: " << ex.what() << std::endl;
    return false;
  }
}

bool ConfigHelper::GetNode(const std::initializer_list<std::string>& areas,
                           YAML::Node& node) const {
  return GetNode(yaml_parser_.GetYAML(), areas, node);
}

bool ConfigHelper::GetNode(const YAML::Node& parent,
                           const std::initializer_list<std::string>& areas, YAML::Node& node) {
  try {
    std::ostringstream path;
    node.reset(parent);
    for (const auto& area : areas) {
      path << area << " ";
      if (node[area]) {
        node.reset(node[area]);
      } else {
        return false;
      }
    }
    return true;
  } catch (std::exception& ex) {
    std::cerr << "get node error: " << ex.what();
    return false;
  }
}

bool ConfigHelper::GetNodes(const std::initializer_list<std::string>& areas,
                            std::vector<std::string>& nodes) const {
  return GetNodes(yaml_parser_.GetYAML(), areas, nodes);
}

bool ConfigHelper::GetNodes(const YAML::Node& parent,
                            const std::initializer_list<std::string>& areas,
                            std::vector<std::string>& nodes) {
  try {
    YAML::Node node;
    if (!GetNode(parent, areas, node)) return false;

    for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
      nodes.push_back(it->first.as<std::string>());
    }
    return true;
  } catch (std::exception& ex) {
    std::cerr << "get nodes error: " << ex.what() << std::endl;
    return false;
  }
}

bool ConfigHelper::FilterNode(const YAML::Node& parent, std::vector<YAML::Node>& nodes,
                              std::function<bool(size_t, const YAML::Node&)> filter) {
  for (size_t idx = 0; idx < parent.size(); ++idx) {
    const auto& child = parent[idx];
    if (filter(idx, child)) {
      YAML::Node node;
      node.reset(child);
      nodes.emplace_back(std::move(node));
    }
  }

  return true;
}

}  // namespace trpc
