#pragma once

#include "trpc/common/config/mysql_client_conf.h"
// #include "trpc/common/config/mysql_connect_pool_conf_parser.h"
#include "yaml-cpp/yaml.h"

namespace YAML {

template <>
struct convert<trpc::MysqlClientConf> {
  static YAML::Node encode(const trpc::MysqlClientConf& mysql_conf) {
    YAML::Node node;
    node["user_name"] = mysql_conf.user_name;
    node["password"] = mysql_conf.password;
    node["dbname"] = mysql_conf.dbname;
    node["char_set"] = mysql_conf.char_set;
    node["thread_num"] = mysql_conf.thread_num;
    node["thread_bind_core"] = mysql_conf.thread_bind_core;
    node["num_shard_group"] = mysql_conf.num_shard_group;
    node["enable"] = mysql_conf.enable;

    return node;
  }

  // 从 YAML 节点解码为 trpc::MysqlClientConf 对象
  static bool decode(const YAML::Node& node, trpc::MysqlClientConf& mysql_conf) {
    if (node["user_name"]) {
      mysql_conf.user_name = node["user_name"].as<std::string>();
    }
    if (node["password"]) {
      mysql_conf.password = node["password"].as<std::string>();
    }
    if (node["dbname"]) {
      mysql_conf.dbname = node["dbname"].as<std::string>();
    }
    if (node["char_set"]) {
      mysql_conf.char_set = node["char_set"].as<std::string>();
    }
    if (node["thread_num"]) {
      mysql_conf.thread_num = node["thread_num"].as<size_t>();
    }
    if (node["thread_bind_core"]) {
      mysql_conf.thread_bind_core = node["thread_bind_core"].as<bool>();
    }
    if (node["num_shard_group"]) {
      mysql_conf.num_shard_group = node["num_shard_group"].as<uint32_t>();
    }
    if (node["enable"]) {
      mysql_conf.enable = node["enable"].as<bool>();
    }

    return true;
  }
};

}  // namespace YAML
