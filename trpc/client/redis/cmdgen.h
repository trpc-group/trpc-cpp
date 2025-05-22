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

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace trpc {

namespace redis {

namespace traits {

template <typename T, typename... Args>
struct back {
  using type = typename back<Args...>::type;
};

template <typename T>
struct back<T> {
  using type = T;
};

template <typename T, typename... Ts>
struct front {
  using type = T;
};

template <typename T1, typename T2, typename... Ts>
struct is_contain {
  static constexpr bool value = std::is_same<T1, T2>::value ? true : is_contain<T1, Ts...>::value;
};

template <typename T1, typename T2>
struct is_contain<T1, T2> {
  static constexpr bool value = std::is_same<T1, T2>::value;
};

template <typename T, typename... Args>
struct is_different_types {
  static constexpr bool value = is_contain<T, Args...>::value ? false : is_different_types<Args...>::value;
};

template <typename T1>
struct is_different_types<T1> {
  static constexpr bool value = true;
};

}  // namespace traits

inline std::string DumpRESP(const std::string& data) {
  std::string escaped_data(data);

  std::string::size_type pos = 0;
  while ((pos = escaped_data.find("\n")) != std::string::npos) {
    escaped_data.replace(pos, 1, "\\n");
  }
  while ((pos = escaped_data.find("\r")) != std::string::npos) {
    escaped_data.replace(pos, 1, "\\r");
  }

  return escaped_data;
}

// cmdgen is the class providing specific RESP generator procedure.
class cmdgen {
  // hello, Here are some APIs, compatible with redis-5.0.0
  // you can check out the details on https://redis.io/commands
  // Generally speaking, the current api is stable, well documented and keep pace with redis
  // upstream. If you want a quick start of them, just have a look at the provided test cases
  // (redis_service_proxy_test, etc.) to see the usage. the testcases cover some representative
  // APIs, exhibited usage. the commands order here is just the same as the official website. module
  // level and command level respectively.
  //
  // As for the naming about this level api for user space, use the lowcase native text provided by
  // redis upstream obviously, underline in some api like cluster_addslots indicate the level of
  // subcommand and major otherwise change to ClusterAddslots, it will make mistake it for a single
  // major command, and Addslots is the lexically pause for google style. and for another example,
  // zrevrangebyscore may be weird once naming like ZrevRangeByScore.
  //
  // Will not quote all specification from redis official site, unless there is something need to be
  // explain, illustrated by Function I/O(major command, arguments, and return structure) briefly.

  //   ####   #       #    #   ####    #####  ######  #####
  //  #    #  #       #    #  #          #    #       #    #
  //  #       #       #    #   ####      #    #####   #    #
  //  #       #       #    #       #     #    #       #####
  //  #    #  #       #    #  #    #     #    #       #   #
  //   ####   ######   ####    ####      #    ######  #    #

 public:
  std::string cluster_addslots(const std::vector<std::string>& slots) {
    return TernaryCmdPacket("CLUSTER", "ADDSLOTS", slots);
  }

  std::string cluster_bumpepoch() { return BinaryCmdPacket("CLUSTER", "BUMPEPOCH"); }

  std::string cluster_count_failure_reports(const std::string& node_id) {
    return TernaryCmdPacket("CLUSTER", "COUNT-FAILURE-REPORTS", node_id);
  }

  std::string cluster_countkeysinslot(const std::string& slot) {
    return TernaryCmdPacket("CLUSTER", "COUNTKEYSINSLOT", slot);
  }

  std::string cluster_delslots(const std::vector<std::string>& slots) {
    return TernaryCmdPacket("CLUSTER", "DELSLOTS", slots);
  }

  std::string cluster_failover() { return BinaryCmdPacket("CLUSTER", "FAILOVER"); }

  std::string cluster_failover(const std::string& mode) { return TernaryCmdPacket("CLUSTER", "FAILOVER", mode); }

  std::string cluster_flushslots() { return BinaryCmdPacket("CLUSTER", "FLUSHSLOTS"); }

  std::string cluster_forget(const std::string& node_id) { return TernaryCmdPacket("CLUSTER", "FORGET", node_id); }

  std::string cluster_getkeysinslot(const std::string& slot, int count) {
    return QuaternaryCmdPacket("CLUSTER", "GETKEYSINSLOT", slot, std::to_string(count));
  }

  std::string cluster_info() { return BinaryCmdPacket("CLUSTER", "INFO"); }

  std::string cluster_keyslot(const std::string& key) { return TernaryCmdPacket("CLUSTER", "KEYSLOT", key); }

  std::string cluster_meet(const std::string& ip, int port) {
    return QuaternaryCmdPacket("CLUSTER", "MEET", ip, std::to_string(port));
  }

  std::string cluster_myid() { return BinaryCmdPacket("CLUSTER", "MYID"); }

  std::string cluster_nodes() { return BinaryCmdPacket("CLUSTER", "NODES"); }

  std::string cluster_replicate(const std::string& node_id) {
    return TernaryCmdPacket("CLUSTER", "REPLICATE", node_id);
  }

  std::string cluster_reset() { return BinaryCmdPacket("CLUSTER", "RESET"); }

  std::string cluster_reset(const std::string& mode) { return TernaryCmdPacket("CLUSTER", "RESET", mode); }

  std::string cluster_saveconfig() { return BinaryCmdPacket("CLUSTER", "SAVECONFIG"); }

  std::string cluster_set_config_epoch(const std::string& config_epoch) {
    return TernaryCmdPacket("CLUSTER", "SET-CONFIG-EPOCH", config_epoch);
  }

  std::string cluster_setslot(const std::string& slot, const std::string& mode) {
    return QuaternaryCmdPacket("CLUSTER", "SETSLOT", slot, mode);
  }

  std::string cluster_setslot(const std::string& slot, const std::string& mode, const std::string& node_id) {
    return PentatonicCmdPacket("CLUSTER", "SETSLOT", slot, mode, node_id);
  }

  std::string cluster_slaves(const std::string& node_id) { return TernaryCmdPacket("CLUSTER", "SLAVES", node_id); }

  std::string cluster_replicas(const std::string& node_id) { return TernaryCmdPacket("CLUSTER", "REPLICAS", node_id); }

  std::string cluster_slots() { return BinaryCmdPacket("CLUSTER", "SLOTS"); }

  std::string readonly() { return UnaryCmdPacket(__func__); }

  std::string readwrite() { return UnaryCmdPacket(__func__); }

  //   ####    ####   #    #  #    #  ######   ####    #####     #     ####   #    #
  //  #    #  #    #  ##   #  ##   #  #       #    #     #       #    #    #  ##   #
  //  #       #    #  # #  #  # #  #  #####   #          #       #    #    #  # #  #
  //  #       #    #  #  # #  #  # #  #       #          #       #    #    #  #  # #
  //  #    #  #    #  #   ##  #   ##  #       #    #     #       #    #    #  #   ##
  //   ####    ####   #    #  #    #  ######   ####      #       #     ####   #    #

 public:
  std::string auth(const std::string& user_name, const std::string& password) {
    if (user_name.empty()) {
      return BinaryCmdPacket(__func__, password);
    }
    return TernaryCmdPacket(__func__, user_name, password);
  }

  std::string auth(const std::string& password) { return BinaryCmdPacket(__func__, password); }

  std::string echo(const std::string& message) { return BinaryCmdPacket(__func__, message); }

  std::string ping() { return UnaryCmdPacket(__func__); }

  std::string ping(const std::string& message) { return BinaryCmdPacket(__func__, message); }

  std::string quit() { return UnaryCmdPacket(__func__); }

  std::string select(int index) { return BinaryCmdPacket(__func__, std::to_string(index)); }

  std::string swapdb(int index1, int index2) {
    return TernaryCmdPacket(__func__, std::to_string(index1), std::to_string(index2));
  }

  //   ####   ######   ####
  //  #    #  #       #    #
  //  #       #####   #    #
  //  #  ###  #       #    #
  //  #    #  #       #    #
  //   ####   ######   ####

 public:
  // it will influence the user space, use the bare unit for geo, rather than k-prefixed constant
  // name.
  enum class geo_unit { m, km, ft, mi };

  std::string GeoUnitToString(const geo_unit unit) const {
    switch (unit) {
      case geo_unit::m:
        return "m";
      case geo_unit::km:
        return "km";
      case geo_unit::ft:
        return "ft";
      case geo_unit::mi:
        return "mi";
      default:
        return "";
    }
  }

  std::string geoadd(const std::string& key,
                     const std::vector<std::tuple<std::string, std::string, std::string>>& longitude_latitude_members) {
    std::vector<std::string> cmd;
    cmd.reserve(3 * longitude_latitude_members.size());
    for (const auto& longitude_latitude_member : longitude_latitude_members) {
      cmd.push_back(std::get<0>(longitude_latitude_member));
      cmd.push_back(std::get<1>(longitude_latitude_member));
      cmd.push_back(std::get<2>(longitude_latitude_member));
    }
    return TernaryCmdPacket(__func__, key, cmd);
  }

  std::string geohash(const std::string& key, const std::vector<std::string>& members) {
    return TernaryCmdPacket(__func__, key, members);
  }

  std::string geopos(const std::string& key, const std::vector<std::string>& members) {
    return TernaryCmdPacket(__func__, key, members);
  }

  std::string geodist(const std::string& key, const std::string& member1, const std::string& member2) {
    return QuaternaryCmdPacket(__func__, key, member1, member2);
  }

  std::string geodist(const std::string& key, const std::string& member1, const std::string& member2,
                      const geo_unit unit) {
    return PentatonicCmdPacket(__func__, key, member1, member2, GeoUnitToString(unit));
  }

  std::string georadius(const std::string& key, double longitude, double latitude, double radius, const geo_unit unit,
                        bool withcoord, bool withdist, bool withhash, bool asc) {
    return georadius(key, longitude, latitude, radius, unit, withcoord, withdist, withhash, asc, 0, "", "");
  }

  std::string georadius(const std::string& key, double longitude, double latitude, double radius, const geo_unit unit,
                        bool withcoord, bool withdist, bool withhash, bool asc, std::size_t count) {
    return georadius(key, longitude, latitude, radius, unit, withcoord, withdist, withhash, asc, count, "", "");
  }

  std::string georadius(const std::string& key, double longitude, double latitude, double radius, const geo_unit unit,
                        bool withcoord, bool withdist, bool withhash, bool asc, const std::string& store_key) {
    return georadius(key, longitude, latitude, radius, unit, withcoord, withdist, withhash, asc, 0, store_key, "");
  }

  std::string georadius(const std::string& key, double longitude, double latitude, double radius, const geo_unit unit,
                        bool withcoord, bool withdist, bool withhash, bool asc, const std::string& store_key,
                        const std::string& storedist_key) {
    return georadius(key, longitude, latitude, radius, unit, withcoord, withdist, withhash, asc, 0, store_key,
                     storedist_key);
  }

  std::string georadius(const std::string& key, double longitude, double latitude, double radius, const geo_unit unit,
                        bool withcoord, bool withdist, bool withhash, bool asc, std::size_t count,
                        const std::string& store_key) {
    return georadius(key, longitude, latitude, radius, unit, withcoord, withdist, withhash, asc, count, store_key, "");
  }

  std::string georadius(const std::string& key, double longitude, double latitude, double radius, const geo_unit unit,
                        bool withcoord, bool withdist, bool withhash, bool asc, std::size_t count,
                        const std::string& store_key, const std::string& storedist_key) {
    std::vector<std::string> cmds;
    cmds.push_back(key);
    cmds.push_back(std::to_string(longitude));
    cmds.push_back(std::to_string(latitude));
    cmds.push_back(std::to_string(radius));
    cmds.push_back(GeoUnitToString(unit));

    if (withcoord) {
      cmds.push_back("WITHCOORD");
    }

    if (withdist) {
      cmds.push_back("WITHDIST");
    }

    if (withhash) {
      cmds.push_back("WITHHASH");
    }

    asc ? cmds.push_back("ASC") : cmds.push_back("DESC");

    if (count > 0) {
      cmds.push_back("COUNT");
      cmds.push_back(std::to_string(count));
    }

    if (!store_key.empty()) {
      cmds.push_back("STORE");
      cmds.push_back(store_key);
    }

    if (!storedist_key.empty()) {
      cmds.push_back("STOREDIST");
      cmds.push_back(storedist_key);
    }

    return BinaryCmdPacket(__func__, cmds);
  }

  std::string georadiusbymember(const std::string& key, const std::string& member, double radius, const geo_unit unit,
                                bool withcoord, bool withdist, bool withhash, bool asc) {
    return georadiusbymember(key, member, radius, unit, withcoord, withdist, withhash, asc, 0, "", "");
  }

  std::string georadiusbymember(const std::string& key, const std::string& member, double radius, const geo_unit unit,
                                bool withcoord, bool withdist, bool withhash, bool asc, std::size_t count) {
    return georadiusbymember(key, member, radius, unit, withcoord, withdist, withhash, asc, count, "", "");
  }

  std::string georadiusbymember(const std::string& key, const std::string& member, double radius, const geo_unit unit,
                                bool withcoord, bool withdist, bool withhash, bool asc, const std::string& store_key) {
    return georadiusbymember(key, member, radius, unit, withcoord, withdist, withhash, asc, 0, store_key, "");
  }

  std::string georadiusbymember(const std::string& key, const std::string& member, double radius, const geo_unit unit,
                                bool withcoord, bool withdist, bool withhash, bool asc, const std::string& store_key,
                                const std::string& storedist_key) {
    return georadiusbymember(key, member, radius, unit, withcoord, withdist, withhash, asc, 0, store_key,
                             storedist_key);
  }

  std::string georadiusbymember(const std::string& key, const std::string& member, double radius, const geo_unit unit,
                                bool withcoord, bool withdist, bool withhash, bool asc, std::size_t count,
                                const std::string& store_key) {
    return georadiusbymember(key, member, radius, unit, withcoord, withdist, withhash, asc, count, store_key, "");
  }

  std::string georadiusbymember(const std::string& key, const std::string& member, double radius, const geo_unit unit,
                                bool withcoord, bool withdist, bool withhash, bool asc, std::size_t count,
                                const std::string& store_key, const std::string& storedist_key) {
    std::vector<std::string> cmds;
    cmds.push_back(key);
    cmds.push_back(member);
    cmds.push_back(std::to_string(radius));
    cmds.push_back(GeoUnitToString(unit));

    if (withcoord) {
      cmds.push_back("WITHCOORD");
    }

    if (withdist) {
      cmds.push_back("WITHDIST");
    }

    if (withhash) {
      cmds.push_back("WITHHASH");
    }

    asc ? cmds.push_back("ASC") : cmds.push_back("DESC");

    if (count > 0) {
      cmds.push_back("COUNT");
      cmds.push_back(std::to_string(count));
    }

    if (!store_key.empty()) {
      cmds.push_back("STORE");
      cmds.push_back(store_key);
    }

    if (!storedist_key.empty()) {
      cmds.push_back("STOREDIST");
      cmds.push_back(storedist_key);
    }

    return BinaryCmdPacket(__func__, cmds);
  }

  //  #    #    ##     ####   #    #  ######   ####
  //  #    #   #  #   #       #    #  #       #
  //  ######  #    #   ####   ######  #####    ####
  //  #    #  ######       #  #    #  #            #
  //  #    #  #    #  #    #  #    #  #       #    #
  //  #    #  #    #   ####   #    #  ######   ####

 public:
  std::string hdel(const std::string& key, const std::vector<std::string>& fields) {
    return TernaryCmdPacket(__func__, key, fields);
  }

  std::string hexists(const std::string& key, const std::string& field) {
    return TernaryCmdPacket(__func__, key, field);
  }

  std::string hget(const std::string& key, const std::string& field) { return TernaryCmdPacket(__func__, key, field); }

  std::string hgetall(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string hincrby(const std::string& key, const std::string& field, int increment) {
    return QuaternaryCmdPacket(__func__, key, field, std::to_string(increment));
  }

  std::string hincrbyfloat(const std::string& key, const std::string& field, float increment) {
    return QuaternaryCmdPacket(__func__, key, field, std::to_string(increment));
  }

  std::string hkeys(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string hlen(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string hmget(const std::string& key, const std::vector<std::string>& fields) {
    return TernaryCmdPacket(__func__, key, fields);
  }

  std::string hmset(const std::string& key, const std::vector<std::pair<std::string, std::string>>& field_values) {
    return TernaryCmdPacket(__func__, key, field_values);
  }

  std::string hset(const std::string& key, const std::string& filed, const std::string& value) {
    return QuaternaryCmdPacket(__func__, key, filed, value);
  }

  std::string hsetnx(const std::string& key, const std::string& field, const std::string& value) {
    return QuaternaryCmdPacket(__func__, key, field, value);
  }

  std::string hstrlen(const std::string& key, const std::string& field) {
    return TernaryCmdPacket(__func__, key, field);
  }

  std::string hvals(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string hscan(const std::string& key, std::size_t cursor) {
    return TernaryCmdPacket(__func__, key, std::to_string(cursor));
  }

  std::string hscan(const std::string& key, std::size_t cursor, const std::string& pattern) {
    return PentatonicCmdPacket(__func__, key, std::to_string(cursor), "MATCH", pattern);
  }

  std::string hscan(const std::string& key, std::size_t cursor, std::size_t count) {
    return PentatonicCmdPacket(__func__, key, std::to_string(cursor), "COUNT", std::to_string(count));
  }

  std::string hscan(const std::string& key, std::size_t cursor, const std::string& pattern, std::size_t count) {
    if (pattern.empty() && count <= 0) {
      return hscan(key, cursor);
    }

    if (pattern.empty()) {
      return hscan(key, cursor, count);
    }

    if (count <= 0) {
      return hscan(key, cursor, pattern);
    }

    return HeptatomicCmdPacket(__func__, key, std::to_string(cursor), "MATCH", pattern, "COUNT", std::to_string(count));
  }

  //  #    #   #   #  #####   ######  #####   #        ####    ####   #        ####    ####
  //  #    #    # #   #    #  #       #    #  #       #    #  #    #  #       #    #  #    #
  //  ######     #    #    #  #####   #    #  #       #    #  #       #       #    #  #
  //  #    #     #    #####   #       #####   #       #    #  #  ###  #       #    #  #  ###
  //  #    #     #    #       #       #   #   #       #    #  #    #  #       #    #  #    #
  //  #    #     #    #       ######  #    #  ######   ####    ####   ######   ####    ####

 public:
  std::string pfadd(const std::string& key, const std::vector<std::string>& elements) {
    return TernaryCmdPacket(__func__, key, elements);
  }

  std::string pfcount(const std::vector<std::string>& keys) { return BinaryCmdPacket(__func__, keys); }

  std::string pfmerge(const std::string& destkey, const std::vector<std::string>& sourcekeys) {
    return TernaryCmdPacket(__func__, destkey, sourcekeys);
  }

  //  #    #  ######   #   #   ####
  //  #   #   #         # #   #
  //  ####    #####      #     ####
  //  #  #    #          #         #
  //  #   #   #          #    #    #
  //  #    #  ######     #     ####

 public:
  std::string del(const std::vector<std::string>& keys) { return BinaryCmdPacket(__func__, keys); }

  std::string dump(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string exists(const std::vector<std::string>& keys) { return BinaryCmdPacket(__func__, keys); }

  std::string expire(const std::string& key, int seconds) {
    return TernaryCmdPacket(__func__, key, std::to_string(seconds));
  }

  std::string expireat(const std::string& key, int timestamp) {
    return TernaryCmdPacket(__func__, key, std::to_string(timestamp));
  }

  std::string keys(const std::string& pattern) { return BinaryCmdPacket(__func__, pattern); }

  std::string migrate(const std::string& host, int port, const std::string& key, const std::string& destination_db,
                      int timeout) {
    return HexahydricCmdPacket(__func__, host, std::to_string(port), key, destination_db, std::to_string(timeout));
  }

  std::string migrate(const std::string& host, int port, const std::string& key, const std::string& destination_db,
                      int timeout, bool copy, bool replace, const std::string& password,
                      const std::vector<std::string>& keys) {
    std::vector<std::string> cmds;
    cmds.push_back(host);
    cmds.push_back(std::to_string(port));
    cmds.push_back(key);
    cmds.push_back(destination_db);
    cmds.push_back(std::to_string(timeout));

    if (copy) {
      cmds.push_back("COPY");
    }
    if (replace) {
      cmds.push_back("REPLACE");
    }
    if (!password.empty()) {
      cmds.push_back(password);
    }
    if (keys.size() != 0) {
      cmds.push_back("KEYS");
      cmds.insert(cmds.end(), keys.begin(), keys.end());
    }

    return BinaryCmdPacket(__func__, cmds);
  }

  std::string move(const std::string& key, const std::string& db) { return TernaryCmdPacket(__func__, key, db); }

  std::string object(const std::string& subcommand, const std::vector<std::string>& arguments) {
    return TernaryCmdPacket(__func__, subcommand, arguments);
  }

  std::string persist(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string pexpire(const std::string& key, int64_t milliseconds) {
    return TernaryCmdPacket(__func__, key, std::to_string(milliseconds));
  }

  std::string pexpireat(const std::string& key, int64_t milliseconds_timestamp) {
    return TernaryCmdPacket(__func__, key, std::to_string(milliseconds_timestamp));
  }

  std::string pttl(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string randomkey() { return UnaryCmdPacket(__func__); }

  std::string rename(const std::string& key, const std::string& newkey) {
    return TernaryCmdPacket(__func__, key, newkey);
  }

  std::string renamenx(const std::string& key, const std::string& newkey) {
    return TernaryCmdPacket(__func__, key, newkey);
  }

  std::string restore(const std::string& key, int ttl, const std::string& serialized_value) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(ttl), serialized_value);
  }

  std::string restore(const std::string& key, int ttl, const std::string& serialized_value, bool replace, bool absttl) {
    return restore(key, ttl, serialized_value, replace, absttl, 0, 0);
  }

  std::string restore(const std::string& key, int ttl, const std::string& serialized_value, bool replace, bool absttl,
                      int seconds, int frequency) {
    std::vector<std::string> cmds;
    cmds.push_back(key);
    cmds.push_back(std::to_string(ttl));
    cmds.push_back(serialized_value);

    if (replace) {
      cmds.push_back("REPLACE");
    }

    if (absttl) {
      cmds.push_back("ABSTTL");
    }

    if (seconds > 0) {
      cmds.push_back("IDLETIME");
      cmds.push_back(std::to_string(seconds));
    }

    if (frequency > 0) {
      cmds.push_back("FREQ");
      cmds.push_back(std::to_string(frequency));
    }

    return BinaryCmdPacket(__func__, cmds);
  }

  std::string sort(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string sort(const std::string& key, const std::vector<std::string>& get_patterns, bool asc, bool alpha) {
    return sort(key, "", false, 0, 0, get_patterns, asc, alpha, "");
  }

  std::string sort(const std::string& key, std::size_t offset, std::size_t count,
                   const std::vector<std::string>& get_patterns, bool asc, bool alpha) {
    return sort(key, "", true, offset, count, get_patterns, asc, alpha, "");
  }

  std::string sort(const std::string& key, const std::string& by_pattern, const std::vector<std::string>& get_patterns,
                   bool asc, bool alpha) {
    return sort(key, by_pattern, false, 0, 0, get_patterns, asc, alpha, "");
  }

  std::string sort(const std::string& key, const std::vector<std::string>& get_patterns, bool asc, bool alpha,
                   const std::string& store_destination) {
    return sort(key, "", false, 0, 0, get_patterns, asc, alpha, store_destination);
  }

  std::string sort(const std::string& key, std::size_t offset, std::size_t count,
                   const std::vector<std::string>& get_patterns, bool asc, bool alpha,
                   const std::string& store_destination) {
    return sort(key, "", true, offset, count, get_patterns, asc, alpha, store_destination);
  }

  std::string sort(const std::string& key, const std::string& by_pattern, const std::vector<std::string>& get_patterns,
                   bool asc, bool alpha, const std::string& store_destination) {
    return sort(key, by_pattern, false, 0, 0, get_patterns, asc, alpha, store_destination);
  }

  std::string sort(const std::string& key, const std::string& by_pattern, std::size_t offset, std::size_t count,
                   const std::vector<std::string>& get_patterns, bool asc, bool alpha) {
    return sort(key, by_pattern, true, offset, count, get_patterns, asc, alpha, "");
  }

  std::string sort(const std::string& key, const std::string& by_pattern, std::size_t offset, std::size_t count,
                   const std::vector<std::string>& get_patterns, bool asc, bool alpha,
                   const std::string& store_destination) {
    return sort(key, by_pattern, true, offset, count, get_patterns, asc, alpha, store_destination);
  }

  std::string touch(const std::vector<std::string>& keys) { return BinaryCmdPacket(__func__, keys); }

  std::string ttl(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string type(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string unlink(const std::vector<std::string>& keys) { return BinaryCmdPacket(__func__, keys); }

  std::string wait(int numreplicas, int timeout) {
    return TernaryCmdPacket(__func__, std::to_string(numreplicas), std::to_string(timeout));
  }

  std::string scan(std::size_t cursor) { return BinaryCmdPacket(__func__, std::to_string(cursor)); }

  std::string scan(std::size_t cursor, const std::string& pattern) {
    return QuaternaryCmdPacket(__func__, std::to_string(cursor), "MATCH", pattern);
  }

  std::string scan(std::size_t cursor, std::size_t count) {
    return QuaternaryCmdPacket(__func__, std::to_string(cursor), "COUNT", std::to_string(count));
  }

  std::string scan(std::size_t cursor, const std::string& pattern, std::size_t count, const std::string& type) {
    std::vector<std::string> cmd;
    if (!pattern.empty()) {
      cmd.push_back("MATCH");
      cmd.push_back(pattern);
    }

    if (count > 0) {
      cmd.push_back("COUNT");
      cmd.push_back(std::to_string(count));
    }

    if (!type.empty()) {
      cmd.push_back("TYPE");
      cmd.push_back(type);
    }

    return TernaryCmdPacket(__func__, std::to_string(cursor), cmd);
  }

 private:
  std::string sort(const std::string& key, const std::string& by_pattern, bool limit, std::size_t offset,
                   std::size_t count, const std::vector<std::string>& get_patterns, bool asc, bool alpha,
                   const std::string& store_destination) {
    std::vector<std::string> cmd;
    if (!by_pattern.empty()) {
      cmd.push_back("BY");
      cmd.push_back(by_pattern);
    }

    if (limit) {
      cmd.push_back("LIMIT");
      cmd.push_back(std::to_string(offset));
      cmd.push_back(std::to_string(count));
    }

    for (const auto& get_pattern : get_patterns) {
      if (get_pattern.empty()) {
        continue;
      }
      cmd.push_back("GET");
      cmd.push_back(get_pattern);
    }

    asc ? cmd.push_back("ASC") : cmd.push_back("DESC");

    if (alpha) {
      cmd.push_back("ALPHA");
    }

    if (!store_destination.empty()) {
      cmd.push_back("STORE");
      cmd.push_back(store_destination);
    }

    return TernaryCmdPacket(__func__, key, cmd);
  }

  //  #          #     ####    #####   ####
  //  #          #    #          #    #
  //  #          #     ####      #     ####
  //  #          #         #     #         #
  //  #          #    #    #     #    #    #
  //  ######     #     ####      #     ####

 public:
  std::string blpop(const std::vector<std::string>& keys, int timeout) {
    return TernaryCmdPacket(__func__, keys, std::to_string(timeout));
  }

  std::string brpop(const std::vector<std::string>& keys, int timeout) {
    return TernaryCmdPacket(__func__, keys, std::to_string(timeout));
  }

  std::string brpoplpush(const std::string& source, const std::string& destination, int timeout) {
    return QuaternaryCmdPacket(__func__, source, destination, std::to_string(timeout));
  }

  std::string lindex(const std::string& key, int index) {
    return TernaryCmdPacket(__func__, key, std::to_string(index));
  }

  std::string linsert(const std::string& key, const std::string& before_or_after, const std::string& pivot,
                      const std::string& element) {
    return PentatonicCmdPacket(__func__, key, before_or_after, pivot, element);
  }

  std::string llen(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string lpop(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  // Starting with Redis version 6.2.0: Added the count argument.
  // More about https://redis.io/commands/lpop/
  std::string lpop(const std::string& key, int count) { return TernaryCmdPacket(__func__, key, std::to_string(count)); }

  std::string lpush(const std::string& key, const std::vector<std::string>& elements) {
    return TernaryCmdPacket(__func__, key, elements);
  }

  std::string lpushx(const std::string& key, const std::vector<std::string>& elements) {
    return TernaryCmdPacket(__func__, key, elements);
  }

  std::string lrange(const std::string& key, int start, int stop) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(stop));
  }

  std::string lrem(const std::string& key, int count, const std::string& element) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(count), element);
  }

  std::string lset(const std::string& key, int index, const std::string& element) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(index), element);
  }

  std::string ltrim(const std::string& key, int start, int stop) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(stop));
  }

  std::string rpop(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string rpoplpush(const std::string& source, const std::string& destination) {
    return TernaryCmdPacket(__func__, source, destination);
  }

  std::string rpush(const std::string& key, const std::vector<std::string>& elements) {
    return TernaryCmdPacket(__func__, key, elements);
  }

  std::string rpushx(const std::string& key, const std::vector<std::string>& elements) {
    return TernaryCmdPacket(__func__, key, elements);
  }

  //  #####   #    #  #####       #    ####   #    #  #####
  //  #    #  #    #  #    #     #    #       #    #  #    #
  //  #    #  #    #  #####     #      ####   #    #  #####
  //  #####   #    #  #    #   #           #  #    #  #    #
  //  #       #    #  #    #  #       #    #  #    #  #    #
  //  #        ####   #####  #         ####    ####   #####

 public:
  std::string psubscribe(const std::vector<std::string>& patterns) { return BinaryCmdPacket(__func__, patterns); }

  std::string pubsub(const std::string& subcommand, const std::vector<std::string>& arguments) {
    return TernaryCmdPacket(__func__, subcommand, arguments);
  }

  std::string publish(const std::string& channel, const std::string& message) {
    return TernaryCmdPacket(__func__, channel, message);
  }

  std::string punsubscribe(const std::vector<std::string>& patterns) { return BinaryCmdPacket(__func__, patterns); }

  std::string subscribe(const std::vector<std::string>& channels) { return BinaryCmdPacket(__func__, channels); }

  std::string unsubscribe() { return UnaryCmdPacket(__func__); }

  std::string unsubscribe(const std::vector<std::string>& channels) { return BinaryCmdPacket(__func__, channels); }

  //   ####    ####   #####      #    #####    #####     #    #    #   ####
  //  #       #    #  #    #     #    #    #     #       #    ##   #  #    #
  //   ####   #       #    #     #    #    #     #       #    # #  #  #
  //       #  #       #####      #    #####      #       #    #  # #  #  ###
  //  #    #  #    #  #   #      #    #          #       #    #   ##  #    #
  //   ####    ####   #    #     #    #          #       #    #    #   ####

 public:
  std::string eval(const std::string& script, int numkeys, const std::vector<std::string>& keys,
                   const std::vector<std::string>& args) {
    return PentatonicCmdPacket(__func__, script, std::to_string(numkeys), keys, args);
  }

  std::string evalsha(const std::string& sha1, int numkeys, const std::vector<std::string>& keys,
                      const std::vector<std::string>& args) {
    return PentatonicCmdPacket(__func__, sha1, std::to_string(numkeys), keys, args);
  }

  std::string script_debug(const std::string& mode) { return TernaryCmdPacket("SCRIPT", "DEBUG", mode); }

  std::string script_exists(const std::vector<std::string>& sha1s) {
    return TernaryCmdPacket("SCRIPT", "EXISTS", sha1s);
  }

  std::string script_flush() { return BinaryCmdPacket("SCRIPT", "FLUSH"); }

  std::string script_kill() { return BinaryCmdPacket("SCRIPT", "KILL"); }

  std::string script_load(const std::string& script) { return TernaryCmdPacket("SCRIPT", "LOAD", script); }

  //   ####   ######  #####   #    #  ######  #####
  //  #       #       #    #  #    #  #       #    #
  //   ####   #####   #    #  #    #  #####   #    #
  //       #  #       #####   #    #  #       #####
  //  #    #  #       #   #    #  #   #       #   #
  //   ####   ######  #    #    ##    ######  #    #

 public:
  std::string bgrewriteaof() { return UnaryCmdPacket(__func__); }

  std::string bgsave() { return UnaryCmdPacket(__func__); }

  std::string client_id() { return BinaryCmdPacket("CLIENT", "ID"); }

  template <typename T, typename... Ts>
  std::string client_kill(const T& arg, const Ts&... args) {
    static_assert(traits::is_different_types<T, Ts...>::value, "How to say, type is weird.");

    static_assert(!(std::is_class<T>::value && std::is_same<T, typename traits::back<T, Ts...>::type>::value),
                  "How to say, type is weird.");

    std::vector<std::string> cmd({"KILL"});

    ClientKillImpl<T, Ts...>(cmd, arg, args...);

    return BinaryCmdPacket("CLIENT", cmd);
  }

  std::string client_list() { return BinaryCmdPacket("CLIENT", "LIST"); }

  std::string client_list(const std::string& type) {
    if (type.empty()) {
      return BinaryCmdPacket("CLIENT", "LIST");
    } else {
      return QuaternaryCmdPacket("CLIENT", "LIST", "TYPE", type);
    }
  }

  std::string client_getname() { return BinaryCmdPacket("CLIENT", "GETNAME"); }

  std::string client_pause(int timeout) { return TernaryCmdPacket("CLIENT", "PAUSE", std::to_string(timeout)); }

  std::string client_reply(const std::string& mode) { return TernaryCmdPacket("CLIENT", "REPLY", mode); }

  std::string client_setname(const std::string& connection_name) {
    return TernaryCmdPacket("CLIENT", "SETNAME", connection_name);
  }

  std::string client_unblock(int client_id) { return TernaryCmdPacket("CLIENT", "UNBLOCK", std::to_string(client_id)); }

  std::string client_unblock(int client_id, const std::string& mode) {
    return QuaternaryCmdPacket("CLIENT", "UNBLOCK", std::to_string(client_id), mode);
  }

  std::string command() { return UnaryCmdPacket("COMMAND"); }

  std::string command_count() { return BinaryCmdPacket("COMMAND", "COUNT"); }

  std::string command_getkeys() { return BinaryCmdPacket("COMMAND", "GETKEYS"); }

  std::string command_info(const std::vector<std::string>& command_names) {
    return TernaryCmdPacket("COMMAND", "COUNT", command_names);
  }

  std::string config_get(const std::string& parameter) { return TernaryCmdPacket("CONFIG", "GET", parameter); }

  std::string config_rewrite() { return BinaryCmdPacket("CONFIG", "REWRITE"); }

  std::string config_set(const std::string& parameter, const std::string& value) {
    return QuaternaryCmdPacket("CONFIG", "SET", parameter, value);
  }

  std::string config_resetstat() { return BinaryCmdPacket("CONFIG", "RESETSTAT"); }

  std::string dbsize() { return UnaryCmdPacket(__func__); }

  std::string debug_object(const std::string& key) { return TernaryCmdPacket("DEBUG", "OBJECT", key); }

  std::string debug_segfault() { return BinaryCmdPacket("DEBUG", "SEGFAULT"); }

  std::string flushall() { return UnaryCmdPacket(__func__); }

  std::string flushall(bool async) {
    if (async) {
      return BinaryCmdPacket(__func__, "ASYNC");
    } else {
      return UnaryCmdPacket(__func__);
    }
  }

  std::string flushdb() { return UnaryCmdPacket(__func__); }

  std::string flushdb(bool async) {
    if (async) {
      return BinaryCmdPacket(__func__, "ASYNC");
    } else {
      return UnaryCmdPacket(__func__);
    }
  }

  std::string info() { return UnaryCmdPacket(__func__); }

  std::string info(const std::string& section) { return BinaryCmdPacket(__func__, section); }

  std::string lolwut() { return UnaryCmdPacket(__func__); }

  std::string lolwut(int version) {
    if (version > 0) {
      return TernaryCmdPacket(__func__, "VERSION", std::to_string(version));
    } else {
      return UnaryCmdPacket(__func__);
    }
  }

  std::string lastsave() { return UnaryCmdPacket(__func__); }

  std::string memory_doctor() { return BinaryCmdPacket("MEMORY", "DOCTOR"); }

  std::string memory_help() { return BinaryCmdPacket("MEMORY", "HELP"); }

  std::string memory_malloc_stats() { return BinaryCmdPacket("MEMORY", "MALLOC-STATS"); }

  std::string memory_purge() { return BinaryCmdPacket("MEMORY", "PURGE"); }

  std::string memory_stats() { return BinaryCmdPacket("MEMORY", "STATS"); }

  std::string memory_usage(const std::string& key) { return TernaryCmdPacket("MEMORY", "USAGE", key); }

  std::string memory_usage(const std::string& key, int count) {
    return PentatonicCmdPacket("MEMORY", "USAGE", key, "SAMPLES", std::to_string(count));
  }

  std::string module_list() { return BinaryCmdPacket("MODULE", "LIST"); }

  std::string module_load(const std::vector<std::string>& paths) { return TernaryCmdPacket("MODULE", "LOAD", paths); }

  std::string module_unload(const std::string& name) { return TernaryCmdPacket("MODULE", "UNLOAD", name); }

  std::string monitor() { return UnaryCmdPacket(__func__); }

  std::string role() { return UnaryCmdPacket(__func__); }

  std::string save() { return UnaryCmdPacket(__func__); }

  std::string shutdown() { return UnaryCmdPacket(__func__); }

  std::string shutdown(const std::string& save) { return BinaryCmdPacket(__func__, save); }

  std::string slaveof(const std::string& host, int port) {
    return TernaryCmdPacket(__func__, host, std::to_string(port));
  }

  std::string replicaof(const std::string& host, int port) {
    return TernaryCmdPacket(__func__, host, std::to_string(port));
  }

  std::string slowlog(const std::string& subcommand) { return BinaryCmdPacket(__func__, subcommand); }

  std::string slowlog(const std::string& subcommand, const std::string& argument) {
    return TernaryCmdPacket(__func__, subcommand, argument);
  }

  std::string sync() { return UnaryCmdPacket(__func__); }

  std::string psync(int replicationid, int offset) {
    return TernaryCmdPacket(__func__, std::to_string(replicationid), std::to_string(offset));
  }

  std::string time() { return UnaryCmdPacket(__func__); }

  std::string latency_doctor() { return BinaryCmdPacket("LATENCY", "DOCTOR"); }

  std::string latency_graph(const std::string& event) { return TernaryCmdPacket("LATENCY", "GRAPH", event); }

  std::string latency_history(const std::string& event) { return TernaryCmdPacket("LATENCY", "HISTORY", event); }

  std::string latency_latest() { return BinaryCmdPacket("LATENCY", "LATEST"); }

  std::string latency_reset() { return BinaryCmdPacket("LATENCY", "RESET"); }

  std::string latency_reset(const std::string& event) { return TernaryCmdPacket("LATENCY", "RESET", event); }

  std::string latency_help() { return BinaryCmdPacket("LATENCY", "HELP"); }

 public:
  // it will influence the user space, use the bare type name for client, rather than k-prefixed
  // constant name.
  enum class client_type { normal, master, pubsub, slave };

  template <typename T>
  typename std::enable_if<std::is_same<T, client_type>::value>::type ClientKillUnpackArg(std::vector<std::string>& cmd,
                                                                                         client_type type) {
    cmd.emplace_back("TYPE");
    std::string type_string;
    switch (type) {
      case client_type::normal:
        type_string = "normal";
        break;
      case client_type::master:
        type_string = "master";
        break;
      case client_type::pubsub:
        type_string = "pubsub";
        break;
      case client_type::slave:
        type_string = "slave";
        break;
    }
    cmd.emplace_back(type_string);
  }

  template <typename T>
  typename std::enable_if<std::is_same<T, bool>::value>::type ClientKillUnpackArg(std::vector<std::string>& cmd,
                                                                                  bool skipme) {
    cmd.emplace_back("SKIPME");
    cmd.emplace_back(skipme ? "yes" : "no");
  }

  template <typename T>
  typename std::enable_if<std::is_same<T, std::string>::value>::type ClientKillUnpackArg(std::vector<std::string>& cmd,
                                                                                         std::string ip_and_port) {
    cmd.emplace_back("ADDR");
    cmd.emplace_back(ip_and_port);
  }

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value>::type ClientKillUnpackArg(std::vector<std::string>& cmd,
                                                                                uint64_t client_id) {
    cmd.emplace_back("ID");
    cmd.emplace_back(std::to_string(client_id));
  }

  template <typename T, typename... Ts>
  void ClientKillImpl(std::vector<std::string>& cmd, const T& arg, const Ts&... args) {
    ClientKillUnpackArg<T>(cmd, arg);
    ClientKillImpl(cmd, args...);
  }

  template <typename T>
  void ClientKillImpl(std::vector<std::string>& redis_cmd, const T& arg) {
    ClientKillUnpackArg<T>(redis_cmd, arg);
  }

  //   ####   ######   #####   ####
  //  #       #          #    #
  //   ####   #####      #     ####
  //       #  #          #         #
  //  #    #  #          #    #    #
  //   ####   ######     #     ####

 public:
  std::string sadd(const std::string& key, const std::vector<std::string>& members) {
    return TernaryCmdPacket(__func__, key, members);
  }

  std::string scard(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string sdiff(const std::vector<std::string>& keys) { return BinaryCmdPacket(__func__, keys); }

  std::string sdiffstore(const std::string& destination, const std::vector<std::string>& keys) {
    return TernaryCmdPacket(__func__, destination, keys);
  }

  std::string sinter(const std::vector<std::string>& keys) { return BinaryCmdPacket(__func__, keys); }

  std::string sinterstore(const std::string& destination, const std::vector<std::string>& keys) {
    return TernaryCmdPacket(__func__, destination, keys);
  }

  std::string sismember(const std::string& key, const std::string& member) {
    return TernaryCmdPacket(__func__, key, member);
  }

  std::string smembers(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string smove(const std::string& source, const std::string& destination, const std::string& member) {
    return QuaternaryCmdPacket(__func__, source, destination, member);
  }

  std::string spop(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string spop(const std::string& key, int count) { return TernaryCmdPacket(__func__, key, std::to_string(count)); }

  std::string srandmember(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string srandmember(const std::string& key, int count) {
    return TernaryCmdPacket(__func__, key, std::to_string(count));
  }

  std::string srem(const std::string& key, const std::vector<std::string>& members) {
    return TernaryCmdPacket(__func__, key, members);
  }

  std::string sunion(const std::vector<std::string>& keys) { return BinaryCmdPacket(__func__, keys); }

  std::string sunionstore(const std::string& destination, const std::vector<std::string>& keys) {
    return TernaryCmdPacket(__func__, destination, keys);
  }

  std::string sscan(const std::string& key, std::size_t cursor) {
    return TernaryCmdPacket(__func__, key, std::to_string(cursor));
  }

  std::string sscan(const std::string& key, std::size_t cursor, const std::string& pattern) {
    return PentatonicCmdPacket(__func__, key, std::to_string(cursor), "MATCH", pattern);
  }

  std::string sscan(const std::string& key, std::size_t cursor, std::size_t count) {
    return PentatonicCmdPacket(__func__, key, std::to_string(cursor), "COUNT", std::to_string(count));
  }

  std::string sscan(const std::string& key, std::size_t cursor, const std::string& pattern, std::size_t count) {
    if (pattern.empty() && count <= 0) {
      return sscan(key, cursor);
    }

    if (pattern.empty()) {
      return sscan(key, cursor, count);
    }

    if (count <= 0) {
      return sscan(key, cursor, pattern);
    }

    return HeptatomicCmdPacket(__func__, key, std::to_string(cursor), "MATCH", pattern, "COUNT", std::to_string(count));
  }

  //   ####    ####   #####    #####  ######  #####            ####   ######   #####    ####
  //  #       #    #  #    #     #    #       #    #          #       #          #     #
  //   ####   #    #  #    #     #    #####   #    #           ####   #####      #      ####
  //       #  #    #  #####      #    #       #    #               #  #          #          #
  //  #    #  #    #  #   #      #    #       #    #          #    #  #          #     #    #
  //   ####    ####   #    #     #    ######  #####            ####   ######     #      ####

 public:
  // it will influence the user space, use the bare method name for aggregate, rather than
  // k-prefixed constant name.
  enum class aggregate_method { sum, min, max, server_default };

  std::string AggregateMethodToString(aggregate_method method) const {
    switch (method) {
      case aggregate_method::sum:
        return "SUM";
      case aggregate_method::min:
        return "MIN";
      case aggregate_method::max:
        return "MAX";
      default:
        return "";
    }
  }

 public:
  std::string bzpopmin(const std::vector<std::string>& keys, int timeout) {
    return TernaryCmdPacket(__func__, keys, std::to_string(timeout));
  }

  std::string bzpopmax(const std::vector<std::string>& keys, int timeout) {
    return TernaryCmdPacket(__func__, keys, std::to_string(timeout));
  }

  std::string zadd(const std::string& key, const std::vector<std::string>& options,
                   const std::multimap<std::string, std::string>& score_members) {
    std::vector<std::string> cmd;
    cmd.reserve(score_members.size() * 2 + options.size());
    cmd.insert(cmd.end(), options.begin(), options.end());
    cmd.reserve(score_members.size() * 2);
    for (auto& score_member : score_members) {
      cmd.push_back(score_member.first);
      cmd.push_back(score_member.second);
    }

    return TernaryCmdPacket(__func__, key, cmd);
  }

  std::string zcard(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string zcount(const std::string& key, int min, int max) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(min), std::to_string(max));
  }

  std::string zcount(const std::string& key, double min, double max) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(min), std::to_string(max));
  }

  std::string zcount(const std::string& key, const std::string& min, const std::string& max) {
    return QuaternaryCmdPacket(__func__, key, min, max);
  }

  std::string zincrby(const std::string& key, int increment, const std::string& member) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(increment), member);
  }

  std::string zincrby(const std::string& key, double increment, const std::string& member) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(increment), member);
  }

  std::string zincrby(const std::string& key, const std::string& increment, const std::string& member) {
    return QuaternaryCmdPacket(__func__, key, increment, member);
  }

  std::string zinterstore(const std::string& destination, std::size_t numkeys, const std::vector<std::string>& keys,
                          const std::vector<std::size_t> weights, aggregate_method method) {
    std::vector<std::string> cmd;
    cmd.reserve(keys.size() + weights.size() + 4);
    cmd.push_back(std::to_string(numkeys));
    cmd.insert(cmd.end(), keys.begin(), keys.end());

    if (!weights.empty()) {
      cmd.push_back("WEIGHTS");

      for (auto weight : weights) {
        cmd.push_back(std::to_string(weight));
      }
    }

    if (method != aggregate_method::server_default) {
      cmd.push_back("AGGREGATE");
      cmd.push_back(AggregateMethodToString(method));
    }

    return TernaryCmdPacket(__func__, destination, cmd);
  }

  std::string zlexcount(const std::string& key, int min, int max) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(min), std::to_string(max));
  }

  std::string zlexcount(const std::string& key, double min, double max) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(min), std::to_string(max));
  }

  std::string zlexcount(const std::string& key, const std::string& min, const std::string& max) {
    return QuaternaryCmdPacket(__func__, key, min, max);
  }

  std::string zpopmax(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string zpopmax(const std::string& key, int count) {
    return TernaryCmdPacket(__func__, key, std::to_string(count));
  }

  std::string zpopmin(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string zpopmin(const std::string& key, int count) {
    return TernaryCmdPacket(__func__, key, std::to_string(count));
  }

  std::string zrange(const std::string& key, int start, int stop) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(stop));
  }

  std::string zrange(const std::string& key, int start, int stop, bool withscores) {
    if (withscores) {
      return PentatonicCmdPacket(__func__, key, std::to_string(start), std::to_string(stop), "WITHSCORES");
    } else {
      return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(stop));
    }
  }

  std::string zrange(const std::string& key, double start, double stop) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(stop));
  }

  std::string zrange(const std::string& key, double start, double stop, bool withscores) {
    if (withscores) {
      return PentatonicCmdPacket(__func__, key, std::to_string(start), std::to_string(stop), "WITHSCORES");
    } else {
      return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(stop));
    }
  }

  std::string zrange(const std::string& key, const std::string& start, const std::string& stop) {
    return QuaternaryCmdPacket(__func__, key, start, stop);
  }

  std::string zrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores) {
    if (withscores) {
      return PentatonicCmdPacket(__func__, key, start, stop, "WITHSCORES");
    } else {
      return QuaternaryCmdPacket(__func__, key, start, stop);
    }
  }

  std::string zrangebylex(const std::string& key, int min, int max) {
    return zrangebylex(key, std::to_string(min), std::to_string(max), false, 0, 0);
  }

  std::string zrangebylex(const std::string& key, double min, double max) {
    return zrangebylex(key, std::to_string(min), std::to_string(max), false, 0, 0);
  }

  std::string zrangebylex(const std::string& key, const std::string& min, const std::string& max) {
    return zrangebylex(key, min, max, false, 0, 0);
  }

  std::string zrangebylex(const std::string& key, int min, int max, std::size_t offset, std::size_t count) {
    return zrangebylex(key, std::to_string(min), std::to_string(max), true, offset, count);
  }

  std::string zrangebylex(const std::string& key, double min, double max, std::size_t offset, std::size_t count) {
    return zrangebylex(key, std::to_string(min), std::to_string(max), true, offset, count);
  }

  std::string zrangebylex(const std::string& key, const std::string& min, const std::string& max, std::size_t offset,
                          std::size_t count) {
    return zrangebylex(key, min, max, true, offset, count);
  }

  std::string zrevrangebylex(const std::string& key, int min, int max) {
    return zrevrangebylex(key, std::to_string(min), std::to_string(max), false, 0, 0);
  }

  std::string zrevrangebylex(const std::string& key, double min, double max) {
    return zrevrangebylex(key, std::to_string(min), std::to_string(max), false, 0, 0);
  }

  std::string zrevrangebylex(const std::string& key, const std::string& min, const std::string& max) {
    return zrevrangebylex(key, min, max, false, 0, 0);
  }

  std::string zrevrangebylex(const std::string& key, int min, int max, std::size_t offset, std::size_t count) {
    return zrevrangebylex(key, std::to_string(min), std::to_string(max), true, offset, count);
  }

  std::string zrevrangebylex(const std::string& key, double min, double max, std::size_t offset, std::size_t count) {
    return zrevrangebylex(key, std::to_string(min), std::to_string(max), true, offset, count);
  }

  std::string zrevrangebylex(const std::string& key, const std::string& min, const std::string& max, std::size_t offset,
                             std::size_t count) {
    return zrevrangebylex(key, min, max, true, offset, count);
  }

  std::string zrangebyscore(const std::string& key, int min, int max) {
    return zrangebyscore(key, std::to_string(min), std::to_string(max), false, 0, 0, false);
  }

  std::string zrangebyscore(const std::string& key, int min, int max, bool withscores) {
    return zrangebyscore(key, std::to_string(min), std::to_string(max), false, 0, 0, withscores);
  }

  std::string zrangebyscore(const std::string& key, double min, double max) {
    return zrangebyscore(key, std::to_string(min), std::to_string(max), false, 0, 0, false);
  }

  std::string zrangebyscore(const std::string& key, double min, double max, bool withscores) {
    return zrangebyscore(key, std::to_string(min), std::to_string(max), false, 0, 0, withscores);
  }

  std::string zrangebyscore(const std::string& key, const std::string& min, const std::string& max) {
    return zrangebyscore(key, min, max, false, 0, 0, false);
  }

  std::string zrangebyscore(const std::string& key, const std::string& min, const std::string& max, bool withscores) {
    return zrangebyscore(key, min, max, false, 0, 0, withscores);
  }

  std::string zrangebyscore(const std::string& key, int min, int max, std::size_t offset, std::size_t count) {
    return zrangebyscore(key, std::to_string(min), std::to_string(max), true, offset, count, false);
  }

  std::string zrangebyscore(const std::string& key, int min, int max, std::size_t offset, std::size_t count,
                            bool withscores) {
    return zrangebyscore(key, std::to_string(min), std::to_string(max), true, offset, count, withscores);
  }

  std::string zrangebyscore(const std::string& key, double min, double max, std::size_t offset, std::size_t count) {
    return zrangebyscore(key, std::to_string(min), std::to_string(max), true, offset, count, false);
  }

  std::string zrangebyscore(const std::string& key, double min, double max, std::size_t offset, std::size_t count,
                            bool withscores) {
    return zrangebyscore(key, std::to_string(min), std::to_string(max), true, offset, count, withscores);
  }

  std::string zrangebyscore(const std::string& key, const std::string& min, const std::string& max, std::size_t offset,
                            std::size_t count) {
    return zrangebyscore(key, min, max, true, offset, count, false);
  }

  std::string zrangebyscore(const std::string& key, const std::string& min, const std::string& max, std::size_t offset,
                            std::size_t count, bool withscores) {
    return zrangebyscore(key, min, max, true, offset, count, withscores);
  }

  std::string zrank(const std::string& key, const std::string& member) {
    return TernaryCmdPacket(__func__, key, member);
  }

  std::string zrem(const std::string& key, const std::vector<std::string>& members) {
    return TernaryCmdPacket(__func__, key, members);
  }

  std::string zremrangebylex(const std::string& key, int min, int max) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(min), std::to_string(max));
  }

  std::string zremrangebylex(const std::string& key, double min, double max) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(min), std::to_string(max));
  }

  std::string zremrangebylex(const std::string& key, const std::string& min, const std::string& max) {
    return QuaternaryCmdPacket(__func__, key, min, max);
  }

  std::string zremrangebyrank(const std::string& key, int start, int stop) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(stop));
  }

  std::string zremrangebyrank(const std::string& key, double start, double stop) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(stop));
  }

  std::string zremrangebyrank(const std::string& key, const std::string& start, const std::string& stop) {
    return QuaternaryCmdPacket(__func__, key, start, stop);
  }

  std::string zremrangebyscore(const std::string& key, int min, int max) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(min), std::to_string(max));
  }

  std::string zremrangebyscore(const std::string& key, double min, double max) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(min), std::to_string(max));
  }

  std::string zremrangebyscore(const std::string& key, const std::string& min, const std::string& max) {
    return QuaternaryCmdPacket(__func__, key, min, max);
  }

  std::string zrevrange(const std::string& key, int start, int stop) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(stop));
  }

  std::string zrevrange(const std::string& key, int start, int stop, bool withscores) {
    if (withscores) {
      return PentatonicCmdPacket(__func__, key, std::to_string(start), std::to_string(stop), "WITHSCORES");
    } else {
      return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(stop));
    }
  }

  std::string zrevrange(const std::string& key, double start, double stop) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(stop));
  }

  std::string zrevrange(const std::string& key, double start, double stop, bool withscores) {
    if (withscores) {
      return PentatonicCmdPacket(__func__, key, std::to_string(start), std::to_string(stop), "WITHSCORES");
    } else {
      return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(stop));
    }
  }

  std::string zrevrange(const std::string& key, const std::string& start, const std::string& stop) {
    return QuaternaryCmdPacket(__func__, key, start, stop);
  }

  std::string zrevrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores) {
    if (withscores) {
      return PentatonicCmdPacket(__func__, key, start, stop, "WITHSCORES");
    } else {
      return QuaternaryCmdPacket(__func__, key, start, stop);
    }
  }

  std::string zrevrangebyscore(const std::string& key, int max, int min) {
    return zrevrangebyscore(key, std::to_string(max), std::to_string(min), false, 0, 0, false);
  }

  std::string zrevrangebyscore(const std::string& key, int max, int min, bool withscores) {
    return zrevrangebyscore(key, std::to_string(max), std::to_string(min), false, 0, 0, withscores);
  }

  std::string zrevrangebyscore(const std::string& key, double max, double min) {
    return zrevrangebyscore(key, std::to_string(max), std::to_string(min), false, 0, 0, false);
  }

  std::string zrevrangebyscore(const std::string& key, double max, double min, bool withscores) {
    return zrevrangebyscore(key, std::to_string(max), std::to_string(min), false, 0, 0, withscores);
  }

  std::string zrevrangebyscore(const std::string& key, const std::string& max, const std::string& min) {
    return zrevrangebyscore(key, max, min, false, 0, 0, false);
  }

  std::string zrevrangebyscore(const std::string& key, const std::string& max, const std::string& min,
                               bool withscores) {
    return zrevrangebyscore(key, max, min, false, 0, 0, withscores);
  }

  std::string zrevrangebyscore(const std::string& key, int max, int min, std::size_t offset, std::size_t count) {
    return zrevrangebyscore(key, std::to_string(max), std::to_string(min), true, offset, count, false);
  }

  std::string zrevrangebyscore(const std::string& key, int max, int min, std::size_t offset, std::size_t count,
                               bool withscores) {
    return zrevrangebyscore(key, std::to_string(max), std::to_string(min), true, offset, count, withscores);
  }

  std::string zrevrangebyscore(const std::string& key, double max, double min, std::size_t offset, std::size_t count) {
    return zrevrangebyscore(key, std::to_string(max), std::to_string(min), true, offset, count, false);
  }

  std::string zrevrangebyscore(const std::string& key, double max, double min, std::size_t offset, std::size_t count,
                               bool withscores) {
    return zrevrangebyscore(key, std::to_string(max), std::to_string(min), true, offset, count, withscores);
  }

  std::string zrevrangebyscore(const std::string& key, const std::string& max, const std::string& min,
                               std::size_t offset, std::size_t count) {
    return zrevrangebyscore(key, max, min, true, offset, count, false);
  }

  std::string zrevrangebyscore(const std::string& key, const std::string& max, const std::string& min,
                               std::size_t offset, std::size_t count, bool withscores) {
    return zrevrangebyscore(key, max, min, true, offset, count, withscores);
  }

  std::string zrevrank(const std::string& key, const std::string& member) {
    return TernaryCmdPacket(__func__, key, member);
  }

  std::string zscore(const std::string& key, const std::string& member) {
    return TernaryCmdPacket(__func__, key, member);
  }

  std::string zunionstore(const std::string& destination, std::size_t numkeys, const std::vector<std::string>& keys,
                          const std::vector<std::size_t> weights, aggregate_method method) {
    std::vector<std::string> cmd;
    cmd.reserve(keys.size() + weights.size() + 3);
    cmd.push_back(std::to_string(numkeys));
    cmd.insert(cmd.end(), keys.begin(), keys.end());
    if (!weights.empty()) {
      cmd.push_back("WEIGHTS");
      for (auto& weight : weights) {
        cmd.push_back(std::to_string(weight));
      }
    }

    if (method != aggregate_method::server_default) {
      cmd.push_back("AGGREGATE");
      cmd.push_back(AggregateMethodToString(method));
    }

    return TernaryCmdPacket(__func__, destination, cmd);
  }

  std::string zscan(const std::string& key, std::size_t cursor) {
    return TernaryCmdPacket(__func__, key, std::to_string(cursor));
  }

  std::string zscan(const std::string& key, std::size_t cursor, const std::string& pattern) {
    return PentatonicCmdPacket(__func__, key, std::to_string(cursor), "MATCH", pattern);
  }

  std::string zscan(const std::string& key, std::size_t cursor, std::size_t count) {
    return PentatonicCmdPacket(__func__, key, std::to_string(cursor), "COUNT", std::to_string(count));
  }

  std::string zscan(const std::string& key, std::size_t cursor, const std::string& pattern, std::size_t count) {
    if (pattern.empty() && count <= 0) {
      return zscan(key, cursor);
    }

    if (pattern.empty()) {
      return zscan(key, cursor, count);
    }

    if (count <= 0) {
      return zscan(key, cursor, pattern);
    }

    return HeptatomicCmdPacket(__func__, key, std::to_string(cursor), "MATCH", pattern, "COUNT", std::to_string(count));
  }

 private:
  std::string zrevrangebyscore(const std::string& key, const std::string& max, const std::string& min, bool limit,
                               std::size_t offset, std::size_t count, bool withscores) {
    if (!withscores && !limit) {
      return QuaternaryCmdPacket(__func__, key, max, min);
    }
    if (!withscores) {
      return HeptatomicCmdPacket(__func__, key, max, min, "LIMIT", std::to_string(offset), std::to_string(count));
    }

    if (!limit) {
      return PentatonicCmdPacket(__func__, key, max, min, "WITHSCORES");
    }

    return EightParamCmdPacket(__func__, key, max, min, "WITHSCORES", "LIMIT", std::to_string(offset),
                               std::to_string(count));
  }

  std::string zrangebyscore(const std::string& key, const std::string& min, const std::string& max, bool limit,
                            std::size_t offset, std::size_t count, bool withscores) {
    if (!withscores && !limit) {
      return QuaternaryCmdPacket(__func__, key, min, max);
    }
    if (!withscores) {
      return HeptatomicCmdPacket(__func__, key, min, max, "LIMIT", std::to_string(offset), std::to_string(count));
    }

    if (!limit) {
      return PentatonicCmdPacket(__func__, key, min, max, "WITHSCORES");
    }

    return EightParamCmdPacket(__func__, key, min, max, "WITHSCORES", "LIMIT", std::to_string(offset),
                               std::to_string(count));
  }

  std::string zrevrangebylex(const std::string& key, const std::string& max, const std::string& min, bool limit,
                             std::size_t offset, std::size_t count) {
    if (limit) {
      return HeptatomicCmdPacket(__func__, key, max, min, "LIMIT", std::to_string(offset), std::to_string(count));
    } else {
      return QuaternaryCmdPacket(__func__, key, max, min);
    }
  }

  std::string zrangebylex(const std::string& key, const std::string& min, const std::string& max, bool limit,
                          std::size_t offset, std::size_t count) {
    if (limit) {
      return HeptatomicCmdPacket(__func__, key, min, max, "LIMIT", std::to_string(offset), std::to_string(count));
    } else {
      return QuaternaryCmdPacket(__func__, key, min, max);
    }
  }

  //   ####    #####  #####   ######    ##    #    #   ####
  //  #          #    #    #  #        #  #   ##  ##  #
  //   ####      #    #    #  #####   #    #  # ## #   ####
  //       #     #    #####   #       ######  #    #       #
  //  #    #     #    #   #   #       #    #  #    #  #    #
  //   ####      #    #    #  ######  #    #  #    #   ####

 public:
  //   ####    #####  #####      #    #    #   ####    ####
  //  #          #    #    #     #    ##   #  #    #  #
  //   ####      #    #    #     #    # #  #  #        ####
  //       #     #    #####      #    #  # #  #  ###       #
  //  #    #     #    #   #      #    #   ##  #    #  #    #
  //   ####      #    #    #     #    #    #   ####    ####

 public:
  // it will influence the user space, use the bare type name for overflow, rather than k-prefixed
  // constant name.
  enum class overflow_type { wrap, sat, fail, server_default };

  std::string OverflowTypeToString(overflow_type type) const {
    switch (type) {
      case overflow_type::wrap:
        return "WRAP";
      case overflow_type::sat:
        return "SAT";
      case overflow_type::fail:
        return "FAIL";
      default:
        return "";
    }
  }

 public:
  // it will influence the user space, use the bare type name for bitfield operation, rather than
  // k-prefixed constant name.
  enum class bitfield_operation_type { get, set, incrby };

  std::string BitfieldOperationTypeToString(bitfield_operation_type operation) const {
    switch (operation) {
      case bitfield_operation_type::get:
        return "GET";
      case bitfield_operation_type::set:
        return "SET";
      case bitfield_operation_type::incrby:
        return "INCRBY";
      default:
        return "";
    }
  }

 public:
  struct bitfield_operation {
    bitfield_operation_type operation_type;

    std::string type;

    int offset;

    int value;

    overflow_type overflow;

    static bitfield_operation get(const std::string& type, int offset,
                                  overflow_type overflow = overflow_type::server_default) {
      return {bitfield_operation_type::get, type, offset, 0, overflow};
    }

    static bitfield_operation set(const std::string& type, int offset, int value,
                                  overflow_type overflow = overflow_type::server_default) {
      return {bitfield_operation_type::set, type, offset, value, overflow};
    }

    static bitfield_operation incrby(const std::string& type, int offset, int increment,
                                     overflow_type overflow = overflow_type::server_default) {
      return {bitfield_operation_type::incrby, type, offset, increment, overflow};
    }
  };

 public:
  std::string append(const std::string& key, const std::string& value) {
    return TernaryCmdPacket(__func__, key, value);
  }

  std::string bitcount(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string bitcount(const std::string& key, int start, int end) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(end));
  }

  std::string bitfield(const std::string& key, const std::vector<bitfield_operation>& operations) {
    std::vector<std::string> cmd;

    for (const auto& operation : operations) {
      cmd.push_back(BitfieldOperationTypeToString(operation.operation_type));
      cmd.push_back(operation.type);
      cmd.push_back(std::to_string(operation.offset));

      if (operation.operation_type == bitfield_operation_type::set ||
          operation.operation_type == bitfield_operation_type::incrby) {
        cmd.push_back(std::to_string(operation.value));
      }

      if (operation.overflow != overflow_type::server_default) {
        cmd.push_back("OVERFLOW");
        cmd.push_back(OverflowTypeToString(operation.overflow));
      }
    }

    return TernaryCmdPacket(__func__, key, cmd);
  }

  std::string bitop(const std::string& operation, const std::string& destkey, const std::vector<std::string>& keys) {
    return QuaternaryCmdPacket(__func__, operation, destkey, keys);
  }

  std::string bitpos(const std::string& key, int bit) { return TernaryCmdPacket(__func__, key, std::to_string(bit)); }

  std::string bitpos(const std::string& key, int bit, int start) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(bit), std::to_string(start));
  }

  std::string bitpos(const std::string& key, int bit, int start, int end) {
    return PentatonicCmdPacket(__func__, key, std::to_string(bit), std::to_string(start), std::to_string(end));
  }

  std::string decr(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string decrby(const std::string& key, int decrement) {
    return TernaryCmdPacket(__func__, key, std::to_string(decrement));
  }

  std::string get(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string getbit(const std::string& key, int offset) {
    return TernaryCmdPacket(__func__, key, std::to_string(offset));
  }

  std::string getrange(const std::string& key, int start, int end) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(start), std::to_string(end));
  }

  std::string getset(const std::string& key, const std::string& value) {
    return TernaryCmdPacket(__func__, key, value);
  }

  std::string incr(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  std::string incrby(const std::string& key, int increment) {
    return TernaryCmdPacket(__func__, key, std::to_string(increment));
  }

  std::string incrbyfloat(const std::string& key, float increment) {
    return TernaryCmdPacket(__func__, key, std::to_string(increment));
  }

  std::string mget(const std::vector<std::string>& keys) { return BinaryCmdPacket(__func__, keys); }

  std::string mset(const std::vector<std::pair<std::string, std::string>>& key_values) {
    return BinaryCmdPacket(__func__, key_values);
  }

  std::string msetnx(const std::vector<std::pair<std::string, std::string>>& key_values) {
    return BinaryCmdPacket(__func__, key_values);
  }

  std::string psetex(const std::string& key, int64_t milliseconds, const std::string& value) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(milliseconds), value);
  }

  std::string set(const std::string& key, const std::string& value) { return TernaryCmdPacket(__func__, key, value); }

  std::string set(const std::string& key, const std::string& value, bool ex, int ex_seconds, bool px,
                  int64_t px_milliseconds, bool nx, bool xx) {
    // 10*8 + key.size() + value.size() + 12(ex_seconds)+ 12(px_milliseconds)+ 24("SET", "EX", "PX")
    size_t reserve_len = 80 + 48;
    reserve_len += key.size();
    reserve_len += value.size();
    std::string cmd;
    cmd.reserve(reserve_len);
    size_t param_size = 3;
    if (ex) {
      param_size += 2;
    }
    if (px) {
      param_size += 2;
    }
    if (nx) {
      param_size += 1;
    }
    if (xx) {
      param_size += 1;
    }

    AppendArraySize(param_size, cmd);
    AppendCmd("SET", cmd);
    AppendCmd(key, cmd);
    AppendCmd(value, cmd);
    if (ex) {
      AppendCmd("EX", cmd);
      AppendCmd(std::to_string(ex_seconds), cmd);
    }
    if (px) {
      AppendCmd("PX", cmd);
      AppendCmd(std::to_string(px_milliseconds), cmd);
    }
    if (nx) {
      AppendCmd("NX", cmd);
    }
    if (xx) {
      AppendCmd("XX", cmd);
    }

    return cmd;
  }

  std::string
#ifndef setbit
  setbit
#else
  __setbit
#endif
      (const std::string& key, int offset, const std::string& value) {
    return QuaternaryCmdPacket("setbit", key, std::to_string(offset), value);
  }

  std::string setex(const std::string& key, int seconds, const std::string& value) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(seconds), value);
  }

  std::string setnx(const std::string& key, const std::string& value) { return TernaryCmdPacket(__func__, key, value); }

  std::string setrange(const std::string& key, int offset, const std::string& value) {
    return QuaternaryCmdPacket(__func__, key, std::to_string(offset), value);
  }

  std::string strlen(const std::string& key) { return BinaryCmdPacket(__func__, key); }

  // #####  #####     ##    #    #   ####     ##     ####    #####     #     ####    #    #
  //   #    #    #   #  #   ##   #  #        #  #   #    #     #       #    #    #   ##   #
  //   #    #    #  #    #  # #  #   ####   #    #  #          #       #    #    #   # #  #
  //   #    #####   ######  #  # #       #  ######  #          #       #    #    #   #  # #
  //   #    #   #   #    #  #   ##  #    #  #    #  #    #     #       #    #    #   #   ##
  //   #    #    #  #    #  #    #   ####   #    #   ####      #       #     ####    #    #

 public:
  std::string discard() { return UnaryCmdPacket(__func__); }

  std::string exec() { return UnaryCmdPacket(__func__); }

  std::string multi() { return UnaryCmdPacket(__func__); }

  std::string unwatch() { return UnaryCmdPacket(__func__); }

  std::string watch(const std::vector<std::string>& keys) { return BinaryCmdPacket(__func__, keys); }

 public:
  std::string sentinel_ckquorum(const std::string& name) { return TernaryCmdPacket("SENTINEL", "CKQUORUM", name); }

  std::string sentinel_failover(const std::string& name) { return TernaryCmdPacket("SENTINEL", "FAILOVER", name); }

  std::string sentinel_flushconfig() { return BinaryCmdPacket("SENTINEL", "FLUSHCONFIG"); }

  std::string sentinel_master(const std::string& name) { return TernaryCmdPacket("SENTINEL", "MASTER", name); }

  std::string sentinel_masters() { return BinaryCmdPacket("SENTINEL", "MASTERS"); }

  std::string sentinel_monitor(const std::string& name, const std::string& ip, std::size_t port, std::size_t quorum) {
    return HexahydricCmdPacket("SENTINEL", "MONITOR", name, ip, std::to_string(port), std::to_string(quorum));
  }

  std::string sentinel_ping() { return BinaryCmdPacket("SENTINEL", "PING"); }

  std::string sentinel_remove(const std::string& name) { return TernaryCmdPacket("SENTINEL", "REMOVE", name); }

  std::string sentinel_reset(const std::string& pattern) { return TernaryCmdPacket("SENTINEL", "RESET", pattern); }

  std::string sentinel_sentinels(const std::string& name) { return TernaryCmdPacket("SENTINEL", "SENTINELS", name); }

  std::string sentinel_set(const std::string& name, const std::string& option, const std::string& value) {
    return PentatonicCmdPacket("SENTINEL", "SET", name, option, value);
  }

  std::string sentinel_slaves(const std::string& name) { return TernaryCmdPacket("SENTINEL", "SLAVES", name); }

  // bloom https://oss.redislabs.com/redisbloom/Bloom_Commands/

  // BF.INFO {key}
  std::string bloom_info(const std::string& key) { return BinaryCmdPacket("bf.info", key); }

  // BF.RESERVE {key} {error_rate} {capacity} [EXPANSION {expansion}] [NONSCALING]
  std::string bloom_reserve(const std::string& key, float error_rate, size_t capacity,
                            const std::optional<size_t>& expansion = std::nullopt, bool nonscaling = false) {
    if ((!expansion) && (!nonscaling)) {
      return QuaternaryCmdPacket("bf.reserve", key, std::to_string(error_rate), std::to_string(capacity));
    }
    std::vector<std::string> options;
    if (expansion) {
      options.emplace_back("expansion");
      options.emplace_back(std::to_string(*expansion));
    }
    if (nonscaling) {
      options.emplace_back("nonscaling");
    }
    return PentatonicCmdPacket("bf.reserve", key, std::to_string(error_rate), std::to_string(capacity), options);
  }

  // BF.ADD {key} {item}
  std::string bloom_add(const std::string& key, const std::string& item) {
    return TernaryCmdPacket("bf.add", key, item);
  }

  // BF.MADD {key} {item ...}
  std::string bloom_madd(const std::string& key, const std::vector<std::string>& items) {
    return TernaryCmdPacket("bf.madd", key, items);
  }

  // BF.INSERT {key} [CAPACITY {cap}] [ERROR {error}] [EXPANSION {expansion}] [NOCREATE]
  // [NONSCALING] ITEMS { item... }
  std::string bloom_insert(const std::string& key, const std::vector<std::string>& items,
                           const std::map<std::string, std::string>& options, bool nocreate = false,
                           bool nonscaling = false) {
    if (options.empty() && (!nocreate) && (!nonscaling)) {
      return QuaternaryCmdPacket("bf.insert", key, "items", items);
    }

    std::vector<std::string> real_options;
    for (auto& [k, v] : options) {
      real_options.push_back(k);
      real_options.push_back(v);
    }

    if (nocreate) {
      real_options.emplace_back("nocreate");
    }

    if (nonscaling) {
      real_options.emplace_back("nonscaling");
    }

    real_options.emplace_back("items");

    return QuaternaryCmdPacket("bf.insert", key, real_options, items);
  }

  // BF.EXISTS {key} {item}
  std::string bloom_exists(const std::string& key, const std::string& item) {
    return TernaryCmdPacket("bf.exists", key, item);
  }

  // BF.MEXISTS {key} {item ...}
  std::string bloom_mexists(const std::string& key, const std::vector<std::string>& items) {
    return TernaryCmdPacket("bf.mexists", key, items);
  }

  // BF.SCANDUMP {key} {iter}
  std::string bloom_scandump(const std::string& key, size_t iter) {
    return TernaryCmdPacket("bf.scandump", key, std::to_string(iter));
  }

  // BF.LOADCHUNK {key} {iter} {data}
  std::string bloom_load_chunk(const std::string& key, size_t iter, const std::string& data) {
    return QuaternaryCmdPacket("bf.loadchunk", key, std::to_string(iter), data);
  }

 protected:
  void AppendCmd(const std::string& key, std::string& cmd) {
    snprintf(tmp_buff, sizeof(tmp_buff), "$%u\r\n", static_cast<uint32_t>(key.size()));
    cmd.append(tmp_buff);
    cmd.append(key);
    cmd.append("\r\n");
  }

  void AppendCmd(const std::vector<std::string>& keys, std::string& cmd) {
    for (auto& key : keys) {
      AppendCmd(key, cmd);
    }
  }

  void AppendCmd(const std::vector<std::pair<std::string, std::string>>& field_values, std::string& cmd) {
    for (auto& field_value : field_values) {
      AppendCmd(field_value.first, cmd);
      AppendCmd(field_value.second, cmd);
    }
  }

  void AppendArraySize(const size_t len, std::string& cmd) {
    snprintf(tmp_buff, sizeof(tmp_buff), "*%u\r\n", static_cast<uint32_t>(len));
    cmd.append(tmp_buff);
  }

  std::string UnaryCmdPacket(const std::string& key) {
    std::string cmd;
    size_t reserve_len = 16 + key.size();
    cmd.reserve(reserve_len);
    cmd.append("*1\r\n");
    AppendCmd(key, cmd);
    return cmd;
  }

  std::string BinaryCmdPacket(const std::string& key1, const std::string& key2) {
    std::string cmd;
    size_t reserve_len = 24 + key1.size();
    reserve_len += key2.size();
    cmd.reserve(reserve_len);
    cmd.append("*2\r\n");
    AppendCmd(key1, cmd);
    AppendCmd(key2, cmd);
    return cmd;
  }

  std::string BinaryCmdPacket(const std::string& key1, const std::vector<std::string>& keys2) {
    std::string cmd;
    size_t reserve_len = 16 + key1.size();
    reserve_len += keys2.size() * 8;
    for_each(keys2.begin(), keys2.end(), [&reserve_len](auto& val) -> void { reserve_len += val.size(); });
    cmd.reserve(reserve_len);
    AppendArraySize(1 + keys2.size(), cmd);
    AppendCmd(key1, cmd);
    AppendCmd(keys2, cmd);
    return cmd;
  }

  std::string BinaryCmdPacket(const std::string& key1,
                              const std::vector<std::pair<std::string, std::string>>& field_values) {
    std::string cmd;
    size_t reserve_len = 16 + key1.size();
    reserve_len += (field_values.size() * 16);
    for_each(field_values.begin(), field_values.end(), [&reserve_len](auto& val) -> void {
      reserve_len += val.first.size();
      reserve_len += val.second.size();
    });
    cmd.reserve(reserve_len);
    AppendArraySize(field_values.size() * 2 + 1, cmd);
    AppendCmd(key1, cmd);
    AppendCmd(field_values, cmd);
    return cmd;
  }

  std::string TernaryCmdPacket(const std::string& key1, const std::string& key2, const std::string& key3) {
    std::string cmd;
    size_t reserve_len = 32 + key1.size();
    reserve_len += key2.size();
    reserve_len += key3.size();
    cmd.reserve(reserve_len);
    cmd.append("*3\r\n");
    AppendCmd(key1, cmd);
    AppendCmd(key2, cmd);
    AppendCmd(key3, cmd);
    return cmd;
  }

  std::string TernaryCmdPacket(const std::string& key1, const std::vector<std::string>& keys2,
                               const std::string& key3) {
    std::string cmd;
    size_t reserve_len = (24 + keys2.size() * 8) + key1.size();
    reserve_len += key3.size();
    for_each(keys2.begin(), keys2.end(), [&reserve_len](auto& val) -> void { reserve_len += val.size(); });
    cmd.reserve(reserve_len);
    AppendArraySize(2 + keys2.size(), cmd);
    AppendCmd(key1, cmd);
    AppendCmd(keys2, cmd);
    AppendCmd(key3, cmd);
    return cmd;
  }

  std::string TernaryCmdPacket(const std::string& key1, const std::string& key2,
                               const std::vector<std::string>& keys3) {
    std::string cmd;
    size_t reserve_len = (24 + keys3.size() * 8) + key1.size();
    reserve_len += key2.size();
    for_each(keys3.begin(), keys3.end(), [&reserve_len](auto& val) -> void { reserve_len += val.size(); });
    cmd.reserve(reserve_len);
    AppendArraySize(2 + keys3.size(), cmd);
    AppendCmd(key1, cmd);
    AppendCmd(key2, cmd);
    AppendCmd(keys3, cmd);
    return cmd;
  }

  std::string TernaryCmdPacket(const std::string& key1, const std::string& key2,
                               const std::vector<std::pair<std::string, std::string>>& field_values) {
    std::string cmd;
    size_t reserve_len = (24 + field_values.size() * 16) + key1.size();
    reserve_len += key2.size();
    for_each(field_values.begin(), field_values.end(), [&reserve_len](auto& val) -> void {
      reserve_len += val.first.size();
      reserve_len += val.second.size();
    });
    cmd.reserve(reserve_len);
    AppendArraySize(field_values.size() * 2 + 2, cmd);
    AppendCmd(key1, cmd);
    AppendCmd(key2, cmd);
    AppendCmd(field_values, cmd);
    return cmd;
  }

  std::string QuaternaryCmdPacket(const std::string& key1, const std::string& key2, const std::string& key3,
                                  const std::string& key4) {
    std::string cmd;
    size_t reserve_len = 40 + key1.size();
    reserve_len += key2.size();
    reserve_len += key3.size();
    reserve_len += key4.size();
    cmd.reserve(reserve_len);
    cmd.append("*4\r\n");
    AppendCmd(key1, cmd);
    AppendCmd(key2, cmd);
    AppendCmd(key3, cmd);
    AppendCmd(key4, cmd);
    return cmd;
  }

  std::string QuaternaryCmdPacket(const std::string& key1, const std::string& key2, const std::string& key3,
                                  const std::vector<std::string>& keys4) {
    std::string cmd;
    size_t reserve_len = 32 + keys4.size() * 8;
    reserve_len += key1.size();
    reserve_len += key2.size();
    reserve_len += key3.size();
    for_each(keys4.begin(), keys4.end(), [&reserve_len](auto& val) -> void { reserve_len += val.size(); });
    cmd.reserve(reserve_len);
    AppendArraySize(keys4.size() + 3, cmd);
    AppendCmd(key1, cmd);
    AppendCmd(key2, cmd);
    AppendCmd(key3, cmd);
    AppendCmd(keys4, cmd);
    return cmd;
  }

  std::string QuaternaryCmdPacket(const std::string& key1, const std::string& key2,
                                  const std::vector<std::string>& keys3, const std::vector<std::string>& keys4) {
    std::string cmd;
    size_t reserve_len = 24 + keys3.size() * 8 + keys4.size() * 8;
    reserve_len += key1.size();
    reserve_len += key2.size();
    for_each(keys3.begin(), keys3.end(), [&reserve_len](auto& val) -> void { reserve_len += val.size(); });
    for_each(keys4.begin(), keys4.end(), [&reserve_len](auto& val) -> void { reserve_len += val.size(); });
    cmd.reserve(reserve_len);
    AppendArraySize(keys3.size() + keys4.size() + 2, cmd);
    AppendCmd(key1, cmd);
    AppendCmd(key2, cmd);
    AppendCmd(keys3, cmd);
    AppendCmd(keys4, cmd);
    return cmd;
  }

  std::string PentatonicCmdPacket(const std::string& key1, const std::string& key2, const std::string& key3,
                                  const std::string& key4, const std::string& key5) {
    std::string cmd;
    size_t reserve_len = 48 + key1.size();
    reserve_len += key2.size();
    reserve_len += key3.size();
    reserve_len += key4.size();
    reserve_len += key5.size();
    cmd.reserve(reserve_len);
    cmd.append("*5\r\n");
    AppendCmd(key1, cmd);
    AppendCmd(key2, cmd);
    AppendCmd(key3, cmd);
    AppendCmd(key4, cmd);
    AppendCmd(key5, cmd);
    return cmd;
  }

  std::string PentatonicCmdPacket(const std::string& key1, const std::string& key2, const std::string& key3,
                                  const std::string& key4, const std::vector<std::string>& keys5) {
    std::string cmd;
    size_t reserve_len = 40 + keys5.size() * 8;
    reserve_len += key1.size();
    reserve_len += key2.size();
    reserve_len += key3.size();
    reserve_len += key4.size();
    for_each(keys5.begin(), keys5.end(), [&reserve_len](auto& val) -> void { reserve_len += val.size(); });
    cmd.reserve(reserve_len);
    AppendArraySize(keys5.size() + 4, cmd);
    AppendCmd(key1, cmd);
    AppendCmd(key2, cmd);
    AppendCmd(key3, cmd);
    AppendCmd(key4, cmd);
    AppendCmd(keys5, cmd);
    return cmd;
  }

  std::string PentatonicCmdPacket(const std::string& key1, const std::string& key2, const std::string& key3,
                                  const std::vector<std::string>& keys4, const std::vector<std::string>& keys5) {
    std::string cmd;
    size_t reserve_len = 32 + keys4.size() * 8 + keys5.size() * 8;
    reserve_len += key1.size();
    reserve_len += key2.size();
    reserve_len += key3.size();
    for_each(keys4.begin(), keys4.end(), [&reserve_len](auto& val) -> void { reserve_len += val.size(); });
    for_each(keys5.begin(), keys5.end(), [&reserve_len](auto& val) -> void { reserve_len += val.size(); });
    cmd.reserve(reserve_len);
    AppendArraySize(keys4.size() + keys5.size() + 3, cmd);
    AppendCmd(key1, cmd);
    AppendCmd(key2, cmd);
    AppendCmd(key3, cmd);
    AppendCmd(keys4, cmd);
    AppendCmd(keys5, cmd);
    return cmd;
  }

  std::string HexahydricCmdPacket(const std::string& key1, const std::string& key2, const std::string& key3,
                                  const std::string& key4, const std::string& key5, const std::string& key6) {
    std::string cmd;
    size_t reserve_len = 56 + key1.size();
    reserve_len += key2.size();
    reserve_len += key3.size();
    reserve_len += key4.size();
    reserve_len += key5.size();
    reserve_len += key6.size();
    cmd.reserve(reserve_len);
    cmd.append("*6\r\n");
    AppendCmd(key1, cmd);
    AppendCmd(key2, cmd);
    AppendCmd(key3, cmd);
    AppendCmd(key4, cmd);
    AppendCmd(key5, cmd);
    AppendCmd(key6, cmd);
    return cmd;
  }

  std::string HeptatomicCmdPacket(const std::string& key1, const std::string& key2, const std::string& key3,
                                  const std::string& key4, const std::string& key5, const std::string& key6,
                                  const std::string& key7) {
    std::string cmd;
    size_t reserve_len = 64 + key1.size();
    reserve_len += key2.size();
    reserve_len += key3.size();
    reserve_len += key4.size();
    reserve_len += key5.size();
    reserve_len += key6.size();
    reserve_len += key7.size();
    cmd.reserve(reserve_len);
    cmd.append("*7\r\n");
    AppendCmd(key1, cmd);
    AppendCmd(key2, cmd);
    AppendCmd(key3, cmd);
    AppendCmd(key4, cmd);
    AppendCmd(key5, cmd);
    AppendCmd(key6, cmd);
    AppendCmd(key7, cmd);
    return cmd;
  }

  std::string EightParamCmdPacket(const std::string& key1, const std::string& key2, const std::string& key3,
                                  const std::string& key4, const std::string& key5, const std::string& key6,
                                  const std::string& key7, const std::string& key8) {
    std::string cmd;
    size_t reserve_len = 72 + key1.size();
    reserve_len += key2.size();
    reserve_len += key3.size();
    reserve_len += key4.size();
    reserve_len += key5.size();
    reserve_len += key6.size();
    reserve_len += key7.size();
    reserve_len += key8.size();
    cmd.reserve(reserve_len);
    cmd.append("*8\r\n");
    AppendCmd(key1, cmd);
    AppendCmd(key2, cmd);
    AppendCmd(key3, cmd);
    AppendCmd(key4, cmd);
    AppendCmd(key5, cmd);
    AppendCmd(key6, cmd);
    AppendCmd(key7, cmd);
    AppendCmd(key8, cmd);
    return cmd;
  }

 protected:
  char tmp_buff[16] = {0};
};

}  // namespace redis

}  // namespace trpc
