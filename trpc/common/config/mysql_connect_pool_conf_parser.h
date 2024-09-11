#pragma once

#include "trpc/common/config/mysql_connect_pool_conf.h"
#include "yaml-cpp/yaml.h"

// 特化 YAML 转换逻辑以支持 trpc::MysqlConnectPoolConf
namespace YAML {

template <>
struct convert<trpc::MysqlConnectPoolConf> {
  // 将 trpc::MysqlConnectPoolConf 对象编码为 YAML 节点
  static YAML::Node encode(const trpc::MysqlConnectPoolConf& pool_conf) {
    YAML::Node node;
    node["min_size"] = pool_conf.min_size;
    node["max_size"] = pool_conf.max_size;
    node["max_idle_time"] = pool_conf.max_idle_time;
    node["timeout"] = pool_conf.timeout;
    return node;
  }

  // 从 YAML 节点解码为 trpc::MysqlConnectPoolConf 对象
  static bool decode(const YAML::Node& node, trpc::MysqlConnectPoolConf& pool_conf) {
    if (node["min_size"]) {
      pool_conf.min_size = node["min_size"].as<uint32_t>();
    }
    if (node["max_size"]) {
      pool_conf.max_size = node["max_size"].as<uint32_t>();
    }
    if (node["max_idle_time"]) {
      pool_conf.max_idle_time = node["max_idle_time"].as<uint32_t>();
    }
    if (node["timeout"]) {
      pool_conf.timeout = node["timeout"].as<uint32_t>();
    }
    return true;
  }
};

}  // namespace YAML