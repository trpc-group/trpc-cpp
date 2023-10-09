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

#ifndef setbit
#define setbit 1
#include "trpc/client/redis/cmdgen.h"
#undef setbit
#else
#include "trpc/client/redis/cmdgen.h"
#endif

#include "gtest/gtest.h"

namespace trpc {

namespace testing {

TEST(CmdgenTest, cluster) {
  std::vector<std::string> slots;
  slots.push_back("abc");
  slots.push_back("bcd");
  EXPECT_EQ("*4\r\n$7\r\nCLUSTER\r\n$8\r\nADDSLOTS\r\n$3\r\nabc\r\n$3\r\nbcd\r\n",
            trpc::redis::cmdgen{}.cluster_addslots(slots));
  EXPECT_EQ("*2\r\n$7\r\nCLUSTER\r\n$9\r\nBUMPEPOCH\r\n",
            trpc::redis::cmdgen{}.cluster_bumpepoch());
  EXPECT_EQ("*3\r\n$7\r\nCLUSTER\r\n$21\r\nCOUNT-FAILURE-REPORTS\r\n$3\r\nabc\r\n",
            trpc::redis::cmdgen{}.cluster_count_failure_reports("abc"));
  EXPECT_EQ("*3\r\n$7\r\nCLUSTER\r\n$15\r\nCOUNTKEYSINSLOT\r\n$3\r\nabc\r\n",
            trpc::redis::cmdgen{}.cluster_countkeysinslot("abc"));
  EXPECT_EQ("*4\r\n$7\r\nCLUSTER\r\n$8\r\nDELSLOTS\r\n$3\r\nabc\r\n$3\r\nbcd\r\n",
            trpc::redis::cmdgen{}.cluster_delslots(slots));
  EXPECT_EQ("*2\r\n$7\r\nCLUSTER\r\n$8\r\nFAILOVER\r\n", trpc::redis::cmdgen{}.cluster_failover());
  EXPECT_EQ("*3\r\n$7\r\nCLUSTER\r\n$8\r\nFAILOVER\r\n$4\r\nmode\r\n",
            trpc::redis::cmdgen{}.cluster_failover("mode"));
  EXPECT_EQ("*2\r\n$7\r\nCLUSTER\r\n$10\r\nFLUSHSLOTS\r\n",
            trpc::redis::cmdgen{}.cluster_flushslots());
  EXPECT_EQ("*3\r\n$7\r\nCLUSTER\r\n$6\r\nFORGET\r\n$3\r\nabc\r\n",
            trpc::redis::cmdgen{}.cluster_forget("abc"));
  EXPECT_EQ("*4\r\n$7\r\nCLUSTER\r\n$13\r\nGETKEYSINSLOT\r\n$3\r\nabc\r\n$1\r\n5\r\n",
            trpc::redis::cmdgen{}.cluster_getkeysinslot("abc", 5));
  EXPECT_EQ("*2\r\n$7\r\nCLUSTER\r\n$4\r\nINFO\r\n", trpc::redis::cmdgen{}.cluster_info());
  EXPECT_EQ("*3\r\n$7\r\nCLUSTER\r\n$7\r\nKEYSLOT\r\n$3\r\nabc\r\n",
            trpc::redis::cmdgen{}.cluster_keyslot("abc"));
  EXPECT_EQ("*4\r\n$7\r\nCLUSTER\r\n$4\r\nMEET\r\n$9\r\n127.0.0.1\r\n$3\r\n655\r\n",
            trpc::redis::cmdgen{}.cluster_meet("127.0.0.1", 655));
  EXPECT_EQ("*2\r\n$7\r\nCLUSTER\r\n$4\r\nMYID\r\n", trpc::redis::cmdgen{}.cluster_myid());
  EXPECT_EQ("*2\r\n$7\r\nCLUSTER\r\n$5\r\nNODES\r\n", trpc::redis::cmdgen{}.cluster_nodes());
  EXPECT_EQ("*3\r\n$7\r\nCLUSTER\r\n$9\r\nREPLICATE\r\n$3\r\nabc\r\n",
            trpc::redis::cmdgen{}.cluster_replicate("abc"));
  EXPECT_EQ("*2\r\n$7\r\nCLUSTER\r\n$5\r\nRESET\r\n", trpc::redis::cmdgen{}.cluster_reset());
  EXPECT_EQ("*3\r\n$7\r\nCLUSTER\r\n$5\r\nRESET\r\n$3\r\nabc\r\n",
            trpc::redis::cmdgen{}.cluster_reset("abc"));
  EXPECT_EQ("*2\r\n$7\r\nCLUSTER\r\n$10\r\nSAVECONFIG\r\n",
            trpc::redis::cmdgen{}.cluster_saveconfig());
  EXPECT_EQ("*3\r\n$7\r\nCLUSTER\r\n$16\r\nSET-CONFIG-EPOCH\r\n$3\r\nabc\r\n",
            trpc::redis::cmdgen{}.cluster_set_config_epoch("abc"));
  EXPECT_EQ("*4\r\n$7\r\nCLUSTER\r\n$7\r\nSETSLOT\r\n$3\r\nabc\r\n$3\r\nbcd\r\n",
            trpc::redis::cmdgen{}.cluster_setslot("abc", "bcd"));
  EXPECT_EQ("*5\r\n$7\r\nCLUSTER\r\n$7\r\nSETSLOT\r\n$3\r\nabc\r\n$3\r\nbcd\r\n$3\r\n123\r\n",
            trpc::redis::cmdgen{}.cluster_setslot("abc", "bcd", "123"));
  EXPECT_EQ("*3\r\n$7\r\nCLUSTER\r\n$6\r\nSLAVES\r\n$3\r\nabc\r\n",
            trpc::redis::cmdgen{}.cluster_slaves("abc"));
  EXPECT_EQ("*3\r\n$7\r\nCLUSTER\r\n$8\r\nREPLICAS\r\n$3\r\nabc\r\n",
            trpc::redis::cmdgen{}.cluster_replicas("abc"));
  EXPECT_EQ("*2\r\n$7\r\nCLUSTER\r\n$5\r\nSLOTS\r\n", trpc::redis::cmdgen{}.cluster_slots());
  EXPECT_EQ("*1\r\n$8\r\nreadonly\r\n", trpc::redis::cmdgen{}.readonly());
  EXPECT_EQ("*1\r\n$9\r\nreadwrite\r\n", trpc::redis::cmdgen{}.readwrite());
}

TEST(CmdgenTest, connection) {
  EXPECT_EQ("*2\r\n$4\r\nauth\r\n$8\r\nmy_redis\r\n", trpc::redis::cmdgen{}.auth("my_redis"));
  EXPECT_EQ("*3\r\n$4\r\nauth\r\n$9\r\nuser_name\r\n$8\r\nmy_redis\r\n",
            trpc::redis::cmdgen{}.auth("user_name", "my_redis"));
  EXPECT_EQ("*2\r\n$4\r\nauth\r\n$8\r\nmy_redis\r\n", trpc::redis::cmdgen{}.auth("", "my_redis"));
  EXPECT_EQ("*2\r\n$4\r\necho\r\n$8\r\nmy_redis\r\n", trpc::redis::cmdgen{}.echo("my_redis"));
  EXPECT_EQ("*1\r\n$4\r\nping\r\n", trpc::redis::cmdgen{}.ping());
  EXPECT_EQ("*2\r\n$4\r\nping\r\n$3\r\nabc\r\n", trpc::redis::cmdgen{}.ping("abc"));
  EXPECT_EQ("*1\r\n$4\r\nquit\r\n", trpc::redis::cmdgen{}.quit());
  EXPECT_EQ("*2\r\n$6\r\nselect\r\n$1\r\n1\r\n", trpc::redis::cmdgen{}.select(1));
  EXPECT_EQ("*3\r\n$6\r\nswapdb\r\n$1\r\n1\r\n$1\r\n2\r\n", trpc::redis::cmdgen{}.swapdb(1, 2));
}

TEST(CmdgenTest, geo) {
  std::vector<std::tuple<std::string, std::string, std::string>> longitude_latitude_members;
  longitude_latitude_members.push_back(
      std::tuple<std::string, std::string, std::string>("abc", "dbe", "des"));
  longitude_latitude_members.push_back(
      std::tuple<std::string, std::string, std::string>("ert", "uie", "opu"));
  EXPECT_EQ(
      "*8\r\n$6\r\ngeoadd\r\n$3\r\nkey\r\n$3\r\nabc\r\n$3\r\ndbe\r\n$3\r\ndes\r\n$3\r\nert\r\n$"
      "3\r\nuie\r\n$3\r\nopu\r\n",
      trpc::redis::cmdgen{}.geoadd("key", longitude_latitude_members));
  std::vector<std::string> members;
  members.push_back("qbc");
  members.push_back("ede");
  EXPECT_EQ("*4\r\n$7\r\ngeohash\r\n$3\r\nkey\r\n$3\r\nqbc\r\n$3\r\nede\r\n",
            trpc::redis::cmdgen{}.geohash("key", members));
  EXPECT_EQ("*4\r\n$6\r\ngeopos\r\n$3\r\nkey\r\n$3\r\nqbc\r\n$3\r\nede\r\n",
            trpc::redis::cmdgen{}.geopos("key", members));
  EXPECT_EQ("*4\r\n$7\r\ngeodist\r\n$3\r\nkey\r\n$3\r\nabn\r\n$3\r\nmji\r\n",
            trpc::redis::cmdgen{}.geodist("key", "abn", "mji"));
  EXPECT_EQ("*5\r\n$7\r\ngeodist\r\n$3\r\nkey\r\n$3\r\nabn\r\n$3\r\nmji\r\n$1\r\nm\r\n",
            trpc::redis::cmdgen{}.geodist("key", "abn", "mji", trpc::redis::cmdgen::geo_unit::m));

  EXPECT_EQ(
      "*7\r\n$9\r\ngeoradius\r\n$3\r\nkey\r\n$8\r\n1.299998\r\n$8\r\n2.000001\r\n$8\r\n4."
      "500009\r\n$2\r\nkm\r\n$4\r\nDESC\r\n",
      trpc::redis::cmdgen{}.georadius("key", 1.299998, 2.000001, 4.500009,
                                      trpc::redis::cmdgen::geo_unit::km, false, false, false,
                                      false));
  EXPECT_EQ(
      "*10\r\n$9\r\ngeoradius\r\n$3\r\nkey\r\n$8\r\n1.299998\r\n$8\r\n2.000001\r\n$8\r\n4."
      "500009\r\n$2\r\nft\r\n$9\r\nWITHCOORD\r\n$8\r\nWITHDIST\r\n$8\r\nWITHHASH\r\n$3\r\nASC\r\n",
      trpc::redis::cmdgen{}.georadius("key", 1.299998, 2.000001, 4.500009,
                                      trpc::redis::cmdgen::geo_unit::ft, true, true, true, true));
  EXPECT_EQ(
      "*12\r\n$9\r\ngeoradius\r\n$3\r\nkey\r\n$8\r\n1.299998\r\n$8\r\n2.000001\r\n$8\r\n4."
      "500009\r\n$2\r\nmi\r\n$9\r\nWITHCOORD\r\n$8\r\nWITHDIST\r\n$8\r\nWITHHASH\r\n$3\r\nASC\r\n$"
      "5\r\nCOUNT\r\n$1\r\n2\r\n",
      trpc::redis::cmdgen{}.georadius("key", 1.299998, 2.000001, 4.500009,
                                      trpc::redis::cmdgen::geo_unit::mi, true, true, true, true,
                                      2));
  EXPECT_EQ(
      "*14\r\n$9\r\ngeoradius\r\n$3\r\nkey\r\n$8\r\n1.299998\r\n$8\r\n2.000001\r\n$8\r\n4."
      "500009\r\n$2\r\nmi\r\n$9\r\nWITHCOORD\r\n$8\r\nWITHDIST\r\n$8\r\nWITHHASH\r\n$3\r\nASC\r\n$"
      "5\r\nSTORE\r\n$3\r\nvfr\r\n$9\r\nSTOREDIST\r\n$3\r\nnji\r\n",
      trpc::redis::cmdgen{}.georadius("key", 1.299998, 2.000001, 4.500009,
                                      trpc::redis::cmdgen::geo_unit::mi, true, true, true, true,
                                      "vfr", "nji"));
  EXPECT_EQ(
      "*14\r\n$9\r\ngeoradius\r\n$3\r\nkey\r\n$8\r\n1.299998\r\n$8\r\n2.000001\r\n$8\r\n4."
      "500009\r\n$2\r\nmi\r\n$9\r\nWITHCOORD\r\n$8\r\nWITHDIST\r\n$8\r\nWITHHASH\r\n$3\r\nASC\r\n"
      "$5\r\nCOUNT\r\n$1\r\n2\r\n$5\r\nSTORE\r\n$3\r\nvfr\r\n",
      trpc::redis::cmdgen{}.georadius("key", 1.299998, 2.000001, 4.500009,
                                      trpc::redis::cmdgen::geo_unit::mi, true, true, true, true, 2,
                                      "vfr"));
  EXPECT_EQ(
      "*16\r\n$9\r\ngeoradius\r\n$3\r\nkey\r\n$8\r\n1.299998\r\n$8\r\n2.000001\r\n$8\r\n4."
      "500009\r\n$2\r\nmi\r\n$9\r\nWITHCOORD\r\n$8\r\nWITHDIST\r\n$8\r\nWITHHASH\r\n$3\r\nASC\r\n"
      "$5\r\nCOUNT\r\n$1\r\n2\r\n$5\r\nSTORE\r\n$3\r\nvfr\r\n$9\r\nSTOREDIST\r\n$3\r\nnji\r\n",
      trpc::redis::cmdgen{}.georadius("key", 1.299998, 2.000001, 4.500009,
                                      trpc::redis::cmdgen::geo_unit::mi, true, true, true, true, 2,
                                      "vfr", "nji"));
  EXPECT_EQ(
      "*6\r\n$17\r\ngeoradiusbymember\r\n$3\r\nkey\r\n$6\r\nmember\r\n$8\r\n4.500009\r\n$"
      "2\r\nmi\r\n$4\r\nDESC\r\n",
      trpc::redis::cmdgen{}.georadiusbymember("key", "member", 4.500009,
                                              trpc::redis::cmdgen::geo_unit::mi, false, false,
                                              false, false));

  EXPECT_EQ(
      "*9\r\n$17\r\ngeoradiusbymember\r\n$3\r\nkey\r\n$6\r\nmember\r\n$8\r\n4.500009\r\n$"
      "2\r\nmi\r\n$9\r\nWITHCOORD\r\n$8\r\nWITHDIST\r\n$8\r\nWITHHASH\r\n$3\r\nASC\r\n",
      trpc::redis::cmdgen{}.georadiusbymember(
          "key", "member", 4.500009, trpc::redis::cmdgen::geo_unit::mi, true, true, true, true));

  EXPECT_EQ(
      "*11\r\n$17\r\ngeoradiusbymember\r\n$3\r\nkey\r\n$6\r\nmember\r\n$8\r\n4.500009\r\n$"
      "2\r\nmi\r\n$9\r\nWITHCOORD\r\n$8\r\nWITHDIST\r\n$8\r\nWITHHASH\r\n$3\r\nASC\r\n$"
      "5\r\nCOUNT\r\n$1\r\n2\r\n",
      trpc::redis::cmdgen{}.georadiusbymember(
          "key", "member", 4.500009, trpc::redis::cmdgen::geo_unit::mi, true, true, true, true, 2));

  EXPECT_EQ(
      "*11\r\n$17\r\ngeoradiusbymember\r\n$3\r\nkey\r\n$6\r\nmember\r\n$8\r\n4.500009\r\n$"
      "2\r\nmi\r\n$9\r\nWITHCOORD\r\n$8\r\nWITHDIST\r\n$8\r\nWITHHASH\r\n$3\r\nASC\r\n$"
      "5\r\nSTORE\r\n$3\r\nabc\r\n",
      trpc::redis::cmdgen{}.georadiusbymember("key", "member", 4.500009,
                                              trpc::redis::cmdgen::geo_unit::mi, true, true, true,
                                              true, "abc"));
  EXPECT_EQ(
      "*13\r\n$17\r\ngeoradiusbymember\r\n$3\r\nkey\r\n$6\r\nmember\r\n$8\r\n4.500009\r\n$"
      "2\r\nmi\r\n$9\r\nWITHCOORD\r\n$8\r\nWITHDIST\r\n$8\r\nWITHHASH\r\n$3\r\nASC\r\n$"
      "5\r\nSTORE\r\n$3\r\nabc\r\n$9\r\nSTOREDIST\r\n$3\r\nbcd\r\n",
      trpc::redis::cmdgen{}.georadiusbymember("key", "member", 4.500009,
                                              trpc::redis::cmdgen::geo_unit::mi, true, true, true,
                                              true, "abc", "bcd"));
  EXPECT_EQ(
      "*13\r\n$17\r\ngeoradiusbymember\r\n$3\r\nkey\r\n$6\r\nmember\r\n$8\r\n4.500009\r\n$"
      "2\r\nmi\r\n$9\r\nWITHCOORD\r\n$8\r\nWITHDIST\r\n$8\r\nWITHHASH\r\n$3\r\nASC\r\n$"
      "5\r\nCOUNT\r\n$1\r\n2\r\n$5\r\nSTORE\r\n$3\r\nabc\r\n",
      trpc::redis::cmdgen{}.georadiusbymember("key", "member", 4.500009,
                                              trpc::redis::cmdgen::geo_unit::mi, true, true, true,
                                              true, 2, "abc"));
  EXPECT_EQ(
      "*15\r\n$17\r\ngeoradiusbymember\r\n$3\r\nkey\r\n$6\r\nmember\r\n$8\r\n4.500009\r\n$"
      "2\r\nmi\r\n$9\r\nWITHCOORD\r\n$8\r\nWITHDIST\r\n$8\r\nWITHHASH\r\n$3\r\nASC\r\n$"
      "5\r\nCOUNT\r\n$1\r\n2\r\n$5\r\nSTORE\r\n$3\r\nabc\r\n$9\r\nSTOREDIST\r\n$3\r\nuis\r\n",
      trpc::redis::cmdgen{}.georadiusbymember("key", "member", 4.500009,
                                              trpc::redis::cmdgen::geo_unit::mi, true, true, true,
                                              true, 2, "abc", "uis"));
}

TEST(CmdgenTest, hashes) {
  std::vector<std::string> fields;
  fields.push_back("field1");
  fields.push_back("field2");
  fields.push_back("field3");
  EXPECT_EQ("*5\r\n$4\r\nhdel\r\n$3\r\nkey\r\n$6\r\nfield1\r\n$6\r\nfield2\r\n$6\r\nfield3\r\n",
            trpc::redis::cmdgen{}.hdel("key", fields));
  EXPECT_EQ("*3\r\n$7\r\nhexists\r\n$3\r\nkey\r\n$6\r\nfield1\r\n",
            trpc::redis::cmdgen{}.hexists("key", "field1"));
  EXPECT_EQ("*3\r\n$4\r\nhget\r\n$3\r\nkey\r\n$6\r\nfield1\r\n",
            trpc::redis::cmdgen{}.hget("key", "field1"));
  EXPECT_EQ("*2\r\n$7\r\nhgetall\r\n$3\r\nkey\r\n", trpc::redis::cmdgen{}.hgetall("key"));
  EXPECT_EQ("*4\r\n$7\r\nhincrby\r\n$3\r\nkey\r\n$6\r\nfield1\r\n$1\r\n1\r\n",
            trpc::redis::cmdgen{}.hincrby("key", "field1", 1));
  EXPECT_EQ("*4\r\n$7\r\nhincrby\r\n$3\r\nkey\r\n$6\r\nfield1\r\n$1\r\n1\r\n",
            trpc::redis::cmdgen{}.hincrby("key", "field1", 1));
  EXPECT_EQ("*4\r\n$12\r\nhincrbyfloat\r\n$3\r\nkey\r\n$6\r\nfield1\r\n$8\r\n1.999999\r\n",
            trpc::redis::cmdgen{}.hincrbyfloat("key", "field1", 1.999999f));
  EXPECT_EQ("*2\r\n$5\r\nhkeys\r\n$3\r\nkey\r\n", trpc::redis::cmdgen{}.hkeys("key"));
  EXPECT_EQ("*2\r\n$4\r\nhlen\r\n$3\r\nkey\r\n", trpc::redis::cmdgen{}.hlen("key"));
  EXPECT_EQ("*5\r\n$5\r\nhmget\r\n$3\r\nkey\r\n$6\r\nfield1\r\n$6\r\nfield2\r\n$6\r\nfield3\r\n",
            trpc::redis::cmdgen{}.hmget("key", fields));

  std::vector<std::pair<std::string, std::string>> field_values;
  field_values.push_back(std::make_pair("field1", "value1"));
  field_values.push_back(std::make_pair("field2", "value2"));
  field_values.push_back(std::make_pair("field3", "value3"));
  EXPECT_EQ(
      "*8\r\n$5\r\nhmset\r\n$3\r\nkey\r\n$6\r\nfield1\r\n$6\r\nvalue1\r\n$6\r\nfield2\r\n$"
      "6\r\nvalue2\r\n$6\r\nfield3\r\n$6\r\nvalue3\r\n",
      trpc::redis::cmdgen{}.hmset("key", field_values));
  EXPECT_EQ("*4\r\n$4\r\nhset\r\n$3\r\nkey\r\n$6\r\nfield1\r\n$6\r\nvalue1\r\n",
            trpc::redis::cmdgen{}.hset("key", "field1", "value1"));
  EXPECT_EQ("*4\r\n$6\r\nhsetnx\r\n$3\r\nkey\r\n$6\r\nfield1\r\n$6\r\nvalue1\r\n",
            trpc::redis::cmdgen{}.hsetnx("key", "field1", "value1"));
  EXPECT_EQ("*3\r\n$7\r\nhstrlen\r\n$3\r\nkey\r\n$6\r\nfield1\r\n",
            trpc::redis::cmdgen{}.hstrlen("key", "field1"));
  EXPECT_EQ("*2\r\n$5\r\nhvals\r\n$3\r\nkey\r\n", trpc::redis::cmdgen{}.hvals("key"));
  EXPECT_EQ("*3\r\n$5\r\nhscan\r\n$3\r\nkey\r\n$3\r\n123\r\n",
            trpc::redis::cmdgen{}.hscan("key", 123));
  EXPECT_EQ("*5\r\n$5\r\nhscan\r\n$3\r\nkey\r\n$3\r\n123\r\n$5\r\nMATCH\r\n$3\r\nabc\r\n",
            trpc::redis::cmdgen{}.hscan("key", 123, "abc"));
  EXPECT_EQ("*5\r\n$5\r\nhscan\r\n$3\r\nkey\r\n$3\r\n123\r\n$5\r\nCOUNT\r\n$1\r\n5\r\n",
            trpc::redis::cmdgen{}.hscan("key", 123, 5));
  EXPECT_EQ(
      "*7\r\n$5\r\nhscan\r\n$3\r\nkey\r\n$3\r\n123\r\n$5\r\nMATCH\r\n$3\r\nabc\r\n$5\r\nCOUNT\r\n$"
      "1\r\n5\r\n",
      trpc::redis::cmdgen{}.hscan("key", 123, "abc", 5));
  EXPECT_EQ("*5\r\n$5\r\nhscan\r\n$3\r\nkey\r\n$3\r\n123\r\n$5\r\nCOUNT\r\n$1\r\n5\r\n",
            trpc::redis::cmdgen{}.hscan("key", 123, "", 5));
  EXPECT_EQ("*5\r\n$5\r\nhscan\r\n$3\r\nkey\r\n$3\r\n123\r\n$5\r\nMATCH\r\n$3\r\nabc\r\n",
            trpc::redis::cmdgen{}.hscan("key", 123, "abc", 0));
}

TEST(CmdgenTest, hyperloglog) {
  std::vector<std::string> elements;
  elements.push_back("element1");
  elements.push_back("element2");
  elements.push_back("element3");
  EXPECT_EQ(
      "*5\r\n$5\r\npfadd\r\n$3\r\nkey\r\n$8\r\nelement1\r\n$8\r\nelement2\r\n$8\r\nelement3\r\n",
      trpc::redis::cmdgen{}.pfadd("key", elements));
  EXPECT_EQ("*4\r\n$7\r\npfcount\r\n$8\r\nelement1\r\n$8\r\nelement2\r\n$8\r\nelement3\r\n",
            trpc::redis::cmdgen{}.pfcount(elements));
  EXPECT_EQ(
      "*5\r\n$7\r\npfmerge\r\n$5\r\nmykey\r\n$8\r\nelement1\r\n$8\r\nelement2\r\n$"
      "8\r\nelement3\r\n",
      trpc::redis::cmdgen{}.pfmerge("mykey", elements));
}

TEST(CmdgenTest, keys) {
  std::vector<std::string> keys;
  keys.push_back("key1");
  keys.push_back("key2");
  keys.push_back("key3");
  EXPECT_EQ("*4\r\n$3\r\ndel\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.del(keys));
  EXPECT_EQ("*2\r\n$4\r\ndump\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.dump("mykey"));
  EXPECT_EQ("*4\r\n$6\r\nexists\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.exists(keys));
  EXPECT_EQ("*3\r\n$6\r\nexpire\r\n$5\r\nmykey\r\n$3\r\n200\r\n",
            trpc::redis::cmdgen{}.expire("mykey", 200));
  EXPECT_EQ("*3\r\n$8\r\nexpireat\r\n$5\r\nmykey\r\n$10\r\n1579988788\r\n",
            trpc::redis::cmdgen{}.expireat("mykey", 1579988788));
  EXPECT_EQ("*2\r\n$4\r\nkeys\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.keys("mykey"));
  EXPECT_EQ(
      "*6\r\n$7\r\nmigrate\r\n$4\r\nhost\r\n$4\r\n1200\r\n$5\r\nmykey\r\n$6\r\ntestdb\r\n$"
      "4\r\n1000\r\n",
      trpc::redis::cmdgen{}.migrate("host", 1200, "mykey", "testdb", 1000));
  EXPECT_EQ(
      "*13\r\n$7\r\nmigrate\r\n$4\r\nhost\r\n$4\r\n1200\r\n$5\r\nmykey\r\n$6\r\ntestdb\r\n$"
      "4\r\n1000\r\n$4\r\nCOPY\r\n$7\r\nREPLACE\r\n$7\r\nmyredis\r\n$4\r\nKEYS\r\n$4\r\nkey1\r\n$"
      "4\r\nkey2\r\n$4\r\nkey3\r\n",
      trpc::redis::cmdgen{}.migrate("host", 1200, "mykey", "testdb", 1000, true, true, "myredis",
                                    keys));
  EXPECT_EQ("*3\r\n$4\r\nmove\r\n$5\r\nmykey\r\n$6\r\ntestdb\r\n",
            trpc::redis::cmdgen{}.move("mykey", "testdb"));
  EXPECT_EQ("*5\r\n$6\r\nobject\r\n$5\r\nmykey\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.object("mykey", keys));
  EXPECT_EQ("*2\r\n$7\r\npersist\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.persist("mykey"));
  EXPECT_EQ("*3\r\n$7\r\npexpire\r\n$5\r\nmykey\r\n$7\r\n1500000\r\n",
            trpc::redis::cmdgen{}.pexpire("mykey", 1500000));
  EXPECT_EQ("*3\r\n$9\r\npexpireat\r\n$5\r\nmykey\r\n$9\r\n150000000\r\n",
            trpc::redis::cmdgen{}.pexpireat("mykey", 150000000));
  EXPECT_EQ("*2\r\n$4\r\npttl\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.pttl("mykey"));
  EXPECT_EQ("*2\r\n$3\r\nttl\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.ttl("mykey"));
  EXPECT_EQ("*1\r\n$9\r\nrandomkey\r\n", trpc::redis::cmdgen{}.randomkey());
  EXPECT_EQ("*3\r\n$6\r\nrename\r\n$5\r\nmykey\r\n$6\r\nnewkey\r\n",
            trpc::redis::cmdgen{}.rename("mykey", "newkey"));
  EXPECT_EQ("*3\r\n$8\r\nrenamenx\r\n$5\r\nmykey\r\n$6\r\nnewkey\r\n",
            trpc::redis::cmdgen{}.renamenx("mykey", "newkey"));
  EXPECT_EQ("*4\r\n$7\r\nrestore\r\n$5\r\nmykey\r\n$3\r\n100\r\n$5\r\nvalue\r\n",
            trpc::redis::cmdgen{}.restore("mykey", 100, "value"));
  EXPECT_EQ("*4\r\n$7\r\nrestore\r\n$5\r\nmykey\r\n$3\r\n200\r\n$6\r\nvalue2\r\n",
            trpc::redis::cmdgen{}.restore("mykey", 200, "value2", false, false));
  EXPECT_EQ(
      "*6\r\n$7\r\nrestore\r\n$5\r\nmykey\r\n$3\r\n200\r\n$6\r\nvalue2\r\n$7\r\nREPLACE\r\n$"
      "6\r\nABSTTL\r\n",
      trpc::redis::cmdgen{}.restore("mykey", 200, "value2", true, true));
  EXPECT_EQ(
      "*10\r\n$7\r\nrestore\r\n$5\r\nmykey\r\n$3\r\n200\r\n$6\r\nvalue2\r\n$7\r\nREPLACE\r\n$"
      "6\r\nABSTTL\r\n$8\r\nIDLETIME\r\n$3\r\n100\r\n$4\r\nFREQ\r\n$2\r\n50\r\n",
      trpc::redis::cmdgen{}.restore("mykey", 200, "value2", true, true, 100, 50));
  EXPECT_EQ("*2\r\n$4\r\nsort\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.sort("mykey"));
  EXPECT_EQ(
      "*9\r\n$4\r\nsort\r\n$5\r\nmykey\r\n$3\r\nGET\r\n$4\r\nkey1\r\n$3\r\nGET\r\n$4\r\nkey2\r\n$"
      "3\r\nGET\r\n$4\r\nkey3\r\n$4\r\nDESC\r\n",
      trpc::redis::cmdgen{}.sort("mykey", keys, false, false));
  EXPECT_EQ(
      "*10\r\n$4\r\nsort\r\n$5\r\nmykey\r\n$3\r\nGET\r\n$4\r\nkey1\r\n$3\r\nGET\r\n$4\r\nkey2\r\n$"
      "3\r\nGET\r\n$4\r\nkey3\r\n$3\r\nASC\r\n$5\r\nALPHA\r\n",
      trpc::redis::cmdgen{}.sort("mykey", keys, true, true));
  EXPECT_EQ(
      "*13\r\n$4\r\nsort\r\n$5\r\nmykey\r\n$5\r\nLIMIT\r\n$2\r\n10\r\n$2\r\n20\r\n$3\r\nGET\r\n$"
      "4\r\nkey1\r\n$3\r\nGET\r\n$4\r\nkey2\r\n$3\r\nGET\r\n$4\r\nkey3\r\n$3\r\nASC\r\n$"
      "5\r\nALPHA\r\n",
      trpc::redis::cmdgen{}.sort("mykey", 10, 20, keys, true, true));
  EXPECT_EQ(
      "*12\r\n$4\r\nsort\r\n$5\r\nmykey\r\n$2\r\nBY\r\n$5\r\nbykey\r\n$3\r\nGET\r\n$4\r\nkey1\r\n$"
      "3\r\nGET\r\n$4\r\nkey2\r\n$3\r\nGET\r\n$4\r\nkey3\r\n$3\r\nASC\r\n$5\r\nALPHA\r\n",
      trpc::redis::cmdgen{}.sort("mykey", "bykey", keys, true, true));
  EXPECT_EQ(
      "*12\r\n$4\r\nsort\r\n$5\r\nmykey\r\n$3\r\nGET\r\n$4\r\nkey1\r\n$3\r\nGET\r\n$4\r\nkey2\r\n$"
      "3\r\nGET\r\n$4\r\nkey3\r\n$3\r\nASC\r\n$5\r\nALPHA\r\n$5\r\nSTORE\r\n$8\r\nstorekey\r\n",
      trpc::redis::cmdgen{}.sort("mykey", keys, true, true, "storekey"));
  EXPECT_EQ(
      "*15\r\n$4\r\nsort\r\n$5\r\nmykey\r\n$5\r\nLIMIT\r\n$2\r\n20\r\n$3\r\n400\r\n$3\r\nGET\r\n$"
      "4\r\nkey1\r\n$3\r\nGET\r\n$4\r\nkey2\r\n$3\r\nGET\r\n$4\r\nkey3\r\n$3\r\nASC\r\n$"
      "5\r\nALPHA\r\n$5\r\nSTORE\r\n$8\r\nstorekey\r\n",
      trpc::redis::cmdgen{}.sort("mykey", 20, 400, keys, true, true, "storekey"));
  EXPECT_EQ(
      "*14\r\n$4\r\nsort\r\n$5\r\nmykey\r\n$2\r\nBY\r\n$5\r\nbykey\r\n$3\r\nGET\r\n$4\r\nkey1\r\n$"
      "3\r\nGET\r\n$4\r\nkey2\r\n$3\r\nGET\r\n$4\r\nkey3\r\n$3\r\nASC\r\n$5\r\nALPHA\r\n$"
      "5\r\nSTORE\r\n$8\r\nstorekey\r\n",
      trpc::redis::cmdgen{}.sort("mykey", "bykey", keys, true, true, "storekey"));
  EXPECT_EQ(
      "*15\r\n$4\r\nsort\r\n$5\r\nmykey\r\n$2\r\nBY\r\n$5\r\nbykey\r\n$5\r\nLIMIT\r\n$2\r\n20\r\n$"
      "2\r\n30\r\n$3\r\nGET\r\n$4\r\nkey1\r\n$3\r\nGET\r\n$4\r\nkey2\r\n$3\r\nGET\r\n$"
      "4\r\nkey3\r\n$3\r\nASC\r\n$5\r\nALPHA\r\n",
      trpc::redis::cmdgen{}.sort("mykey", "bykey", 20, 30, keys, true, true));
  EXPECT_EQ(
      "*17\r\n$4\r\nsort\r\n$5\r\nmykey\r\n$2\r\nBY\r\n$5\r\nbykey\r\n$5\r\nLIMIT\r\n$2\r\n20\r\n$"
      "2\r\n30\r\n$3\r\nGET\r\n$4\r\nkey1\r\n$3\r\nGET\r\n$4\r\nkey2\r\n$3\r\nGET\r\n$"
      "4\r\nkey3\r\n$3\r\nASC\r\n$5\r\nALPHA\r\n$5\r\nSTORE\r\n$8\r\nstorekey\r\n",
      trpc::redis::cmdgen{}.sort("mykey", "bykey", 20, 30, keys, true, true, "storekey"));
  EXPECT_EQ("*4\r\n$5\r\ntouch\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.touch(keys));
  EXPECT_EQ("*2\r\n$4\r\ntype\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.type("mykey"));
  EXPECT_EQ("*4\r\n$6\r\nunlink\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.unlink(keys));
  EXPECT_EQ("*3\r\n$4\r\nwait\r\n$3\r\n100\r\n$4\r\n1000\r\n",
            trpc::redis::cmdgen{}.wait(100, 1000));
  EXPECT_EQ("*2\r\n$4\r\nscan\r\n$3\r\n100\r\n", trpc::redis::cmdgen{}.scan(100));
  EXPECT_EQ("*4\r\n$4\r\nscan\r\n$3\r\n100\r\n$5\r\nCOUNT\r\n$4\r\n1000\r\n",
            trpc::redis::cmdgen{}.scan(100, 1000));
  EXPECT_EQ("*4\r\n$4\r\nscan\r\n$3\r\n100\r\n$5\r\nMATCH\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.scan(100, "mykey"));
  EXPECT_EQ(
      "*8\r\n$4\r\nscan\r\n$3\r\n100\r\n$5\r\nMATCH\r\n$5\r\nmykey\r\n$5\r\nCOUNT\r\n$3\r\n100\r\n$"
      "4\r\nTYPE\r\n$3\r\nint\r\n",
      trpc::redis::cmdgen{}.scan(100, "mykey", 100, "int"));
}

TEST(CmdgenTest, lists) {
  std::vector<std::string> keys;
  keys.push_back("key1");
  keys.push_back("key2");
  keys.push_back("key3");
  EXPECT_EQ("*5\r\n$5\r\nblpop\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.blpop(keys, 100));
  EXPECT_EQ("*5\r\n$5\r\nbrpop\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.brpop(keys, 100));
  EXPECT_EQ("*4\r\n$10\r\nbrpoplpush\r\n$6\r\nmykey1\r\n$6\r\nmykey2\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.brpoplpush("mykey1", "mykey2", 100));
  EXPECT_EQ("*3\r\n$6\r\nlindex\r\n$6\r\nmykey1\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.lindex("mykey1", 100));
  EXPECT_EQ(
      "*5\r\n$7\r\nlinsert\r\n$6\r\nmykey1\r\n$6\r\nmykey2\r\n$6\r\nmykey3\r\n$6\r\nmykey4\r\n",
      trpc::redis::cmdgen{}.linsert("mykey1", "mykey2", "mykey3", "mykey4"));
  EXPECT_EQ("*2\r\n$4\r\nllen\r\n$6\r\nmykey1\r\n", trpc::redis::cmdgen{}.llen("mykey1"));
  EXPECT_EQ("*2\r\n$4\r\nlpop\r\n$6\r\nmykey1\r\n", trpc::redis::cmdgen{}.lpop("mykey1"));
  EXPECT_EQ("*5\r\n$5\r\nlpush\r\n$6\r\nmykey1\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.lpush("mykey1", keys));
  EXPECT_EQ("*5\r\n$6\r\nlpushx\r\n$6\r\nmykey1\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.lpushx("mykey1", keys));
  EXPECT_EQ("*4\r\n$6\r\nlrange\r\n$6\r\nmykey1\r\n$3\r\n100\r\n$3\r\n200\r\n",
            trpc::redis::cmdgen{}.lrange("mykey1", 100, 200));
  EXPECT_EQ("*4\r\n$4\r\nlrem\r\n$6\r\nmykey1\r\n$3\r\n100\r\n$9\r\nmyelement\r\n",
            trpc::redis::cmdgen{}.lrem("mykey1", 100, "myelement"));
  EXPECT_EQ("*4\r\n$4\r\nlset\r\n$6\r\nmykey1\r\n$3\r\n100\r\n$9\r\nmyelement\r\n",
            trpc::redis::cmdgen{}.lset("mykey1", 100, "myelement"));
  EXPECT_EQ("*4\r\n$5\r\nltrim\r\n$6\r\nmykey1\r\n$3\r\n100\r\n$3\r\n200\r\n",
            trpc::redis::cmdgen{}.ltrim("mykey1", 100, 200));
  EXPECT_EQ("*2\r\n$4\r\nrpop\r\n$6\r\nmykey1\r\n", trpc::redis::cmdgen{}.rpop("mykey1"));
  EXPECT_EQ("*3\r\n$9\r\nrpoplpush\r\n$6\r\nmykey1\r\n$6\r\nmykey2\r\n",
            trpc::redis::cmdgen{}.rpoplpush("mykey1", "mykey2"));
  EXPECT_EQ("*5\r\n$5\r\nrpush\r\n$6\r\nmykey1\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.rpush("mykey1", keys));
  EXPECT_EQ("*5\r\n$6\r\nrpushx\r\n$6\r\nmykey1\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.rpushx("mykey1", keys));
}

TEST(CmdgenTest, pubsub) {
  std::vector<std::string> keys;
  keys.push_back("key1");
  keys.push_back("key2");
  keys.push_back("key3");
  EXPECT_EQ("*4\r\n$10\r\npsubscribe\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.psubscribe(keys));
  EXPECT_EQ("*5\r\n$6\r\npubsub\r\n$5\r\nmykey\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.pubsub("mykey", keys));
  EXPECT_EQ("*3\r\n$7\r\npublish\r\n$6\r\nmykey1\r\n$6\r\nmykey2\r\n",
            trpc::redis::cmdgen{}.publish("mykey1", "mykey2"));
  EXPECT_EQ("*4\r\n$12\r\npunsubscribe\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.punsubscribe(keys));
  EXPECT_EQ("*4\r\n$9\r\nsubscribe\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.subscribe(keys));
  EXPECT_EQ("*1\r\n$11\r\nunsubscribe\r\n", trpc::redis::cmdgen{}.unsubscribe());
  EXPECT_EQ("*4\r\n$11\r\nunsubscribe\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.unsubscribe(keys));
}

TEST(CmdgenTest, script) {
  std::vector<std::string> keys1;
  keys1.push_back("key1");
  keys1.push_back("key2");
  keys1.push_back("key3");
  std::vector<std::string> keys2;
  keys2.push_back("key4");
  keys2.push_back("key5");
  keys2.push_back("key6");
  EXPECT_EQ(
      "*9\r\n$4\r\neval\r\n$5\r\nmykey\r\n$3\r\n100\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n$"
      "4\r\nkey4\r\n$4\r\nkey5\r\n$4\r\nkey6\r\n",
      trpc::redis::cmdgen{}.eval("mykey", 100, keys1, keys2));
  EXPECT_EQ(
      "*9\r\n$7\r\nevalsha\r\n$5\r\nmykey\r\n$3\r\n100\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$"
      "4\r\nkey3\r\n$4\r\nkey4\r\n$4\r\nkey5\r\n$4\r\nkey6\r\n",
      trpc::redis::cmdgen{}.evalsha("mykey", 100, keys1, keys2));
  EXPECT_EQ("*3\r\n$6\r\nSCRIPT\r\n$5\r\nDEBUG\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.script_debug("mykey"));
  EXPECT_EQ("*5\r\n$6\r\nSCRIPT\r\n$6\r\nEXISTS\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.script_exists(keys1));
  EXPECT_EQ("*2\r\n$6\r\nSCRIPT\r\n$5\r\nFLUSH\r\n", trpc::redis::cmdgen{}.script_flush());
  EXPECT_EQ("*2\r\n$6\r\nSCRIPT\r\n$4\r\nKILL\r\n", trpc::redis::cmdgen{}.script_kill());
  EXPECT_EQ("*3\r\n$6\r\nSCRIPT\r\n$4\r\nLOAD\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.script_load("mykey"));
}

TEST(CmdgenTest, server) {
  std::vector<std::string> keys1;
  keys1.push_back("key1");
  keys1.push_back("key2");
  keys1.push_back("key3");
  EXPECT_EQ("*1\r\n$12\r\nbgrewriteaof\r\n", trpc::redis::cmdgen{}.bgrewriteaof());
  EXPECT_EQ("*1\r\n$6\r\nbgsave\r\n", trpc::redis::cmdgen{}.bgsave());
  EXPECT_EQ("*2\r\n$6\r\nCLIENT\r\n$2\r\nID\r\n", trpc::redis::cmdgen{}.client_id());
  EXPECT_EQ(
      "*6\r\n$6\r\nCLIENT\r\n$4\r\nKILL\r\n$4\r\nADDR\r\n$6\r\nmykey1\r\n$2\r\nID\r\n$3\r\n100\r\n",
      trpc::redis::cmdgen{}.client_kill(std::string("mykey1"), 100));
  EXPECT_EQ("*2\r\n$6\r\nCLIENT\r\n$4\r\nLIST\r\n", trpc::redis::cmdgen{}.client_list());
  EXPECT_EQ("*4\r\n$6\r\nCLIENT\r\n$4\r\nLIST\r\n$4\r\nTYPE\r\n$3\r\nint\r\n",
            trpc::redis::cmdgen{}.client_list("int"));
  EXPECT_EQ("*2\r\n$6\r\nCLIENT\r\n$7\r\nGETNAME\r\n", trpc::redis::cmdgen{}.client_getname());
  EXPECT_EQ("*3\r\n$6\r\nCLIENT\r\n$5\r\nPAUSE\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.client_pause(100));
  EXPECT_EQ("*3\r\n$6\r\nCLIENT\r\n$5\r\nREPLY\r\n$3\r\nabc\r\n",
            trpc::redis::cmdgen{}.client_reply("abc"));
  EXPECT_EQ("*3\r\n$6\r\nCLIENT\r\n$7\r\nSETNAME\r\n$8\r\nmyclient\r\n",
            trpc::redis::cmdgen{}.client_setname("myclient"));
  EXPECT_EQ("*3\r\n$6\r\nCLIENT\r\n$7\r\nUNBLOCK\r\n$1\r\n1\r\n",
            trpc::redis::cmdgen{}.client_unblock(1));
  EXPECT_EQ("*4\r\n$6\r\nCLIENT\r\n$7\r\nUNBLOCK\r\n$1\r\n1\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.client_unblock(1, "mykey"));
  EXPECT_EQ("*1\r\n$7\r\nCOMMAND\r\n", trpc::redis::cmdgen{}.command());
  EXPECT_EQ("*2\r\n$7\r\nCOMMAND\r\n$5\r\nCOUNT\r\n", trpc::redis::cmdgen{}.command_count());
  EXPECT_EQ("*2\r\n$7\r\nCOMMAND\r\n$7\r\nGETKEYS\r\n", trpc::redis::cmdgen{}.command_getkeys());
  EXPECT_EQ("*5\r\n$7\r\nCOMMAND\r\n$5\r\nCOUNT\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.command_info(keys1));
  EXPECT_EQ("*3\r\n$6\r\nCONFIG\r\n$3\r\nGET\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.config_get("mykey"));
  EXPECT_EQ("*2\r\n$6\r\nCONFIG\r\n$7\r\nREWRITE\r\n", trpc::redis::cmdgen{}.config_rewrite());
  EXPECT_EQ("*4\r\n$6\r\nCONFIG\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$5\r\nvalue\r\n",
            trpc::redis::cmdgen{}.config_set("mykey", "value"));
  EXPECT_EQ("*2\r\n$6\r\nCONFIG\r\n$9\r\nRESETSTAT\r\n", trpc::redis::cmdgen{}.config_resetstat());
  EXPECT_EQ("*1\r\n$6\r\ndbsize\r\n", trpc::redis::cmdgen{}.dbsize());
  EXPECT_EQ("*3\r\n$5\r\nDEBUG\r\n$6\r\nOBJECT\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.debug_object("mykey"));
  EXPECT_EQ("*2\r\n$5\r\nDEBUG\r\n$8\r\nSEGFAULT\r\n", trpc::redis::cmdgen{}.debug_segfault());
  EXPECT_EQ("*1\r\n$8\r\nflushall\r\n", trpc::redis::cmdgen{}.flushall());
  EXPECT_EQ("*2\r\n$8\r\nflushall\r\n$5\r\nASYNC\r\n", trpc::redis::cmdgen{}.flushall(true));
  EXPECT_EQ("*2\r\n$7\r\nflushdb\r\n$5\r\nASYNC\r\n", trpc::redis::cmdgen{}.flushdb(true));
  EXPECT_EQ("*1\r\n$7\r\nflushdb\r\n", trpc::redis::cmdgen{}.flushdb());
  EXPECT_EQ("*1\r\n$4\r\ninfo\r\n", trpc::redis::cmdgen{}.info());
  EXPECT_EQ("*2\r\n$4\r\ninfo\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.info("mykey"));
  EXPECT_EQ("*1\r\n$6\r\nlolwut\r\n", trpc::redis::cmdgen{}.lolwut());
  EXPECT_EQ("*3\r\n$6\r\nlolwut\r\n$7\r\nVERSION\r\n$2\r\n10\r\n",
            trpc::redis::cmdgen{}.lolwut(10));
  EXPECT_EQ("*1\r\n$8\r\nlastsave\r\n", trpc::redis::cmdgen{}.lastsave());
  EXPECT_EQ("*2\r\n$6\r\nMEMORY\r\n$6\r\nDOCTOR\r\n", trpc::redis::cmdgen{}.memory_doctor());
  EXPECT_EQ("*2\r\n$6\r\nMEMORY\r\n$4\r\nHELP\r\n", trpc::redis::cmdgen{}.memory_help());
  EXPECT_EQ("*2\r\n$6\r\nMEMORY\r\n$12\r\nMALLOC-STATS\r\n",
            trpc::redis::cmdgen{}.memory_malloc_stats());
  EXPECT_EQ("*2\r\n$6\r\nMEMORY\r\n$5\r\nPURGE\r\n", trpc::redis::cmdgen{}.memory_purge());
  EXPECT_EQ("*2\r\n$6\r\nMEMORY\r\n$5\r\nSTATS\r\n", trpc::redis::cmdgen{}.memory_stats());
  EXPECT_EQ("*3\r\n$6\r\nMEMORY\r\n$5\r\nUSAGE\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.memory_usage("mykey"));
  EXPECT_EQ("*5\r\n$6\r\nMEMORY\r\n$5\r\nUSAGE\r\n$5\r\nmykey\r\n$7\r\nSAMPLES\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.memory_usage("mykey", 100));
  EXPECT_EQ("*2\r\n$6\r\nMODULE\r\n$4\r\nLIST\r\n", trpc::redis::cmdgen{}.module_list());
  EXPECT_EQ("*5\r\n$6\r\nMODULE\r\n$4\r\nLOAD\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.module_load(keys1));
  EXPECT_EQ("*3\r\n$6\r\nMODULE\r\n$6\r\nUNLOAD\r\n$8\r\nmymodule\r\n",
            trpc::redis::cmdgen{}.module_unload("mymodule"));
  EXPECT_EQ("*1\r\n$7\r\nmonitor\r\n", trpc::redis::cmdgen{}.monitor());
  EXPECT_EQ("*1\r\n$4\r\nrole\r\n", trpc::redis::cmdgen{}.role());
  EXPECT_EQ("*1\r\n$4\r\nsave\r\n", trpc::redis::cmdgen{}.save());
  EXPECT_EQ("*1\r\n$8\r\nshutdown\r\n", trpc::redis::cmdgen{}.shutdown());
  EXPECT_EQ("*2\r\n$8\r\nshutdown\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.shutdown("mykey"));
  EXPECT_EQ("*3\r\n$7\r\nslaveof\r\n$9\r\n127.0.0.1\r\n$4\r\n2300\r\n",
            trpc::redis::cmdgen{}.slaveof("127.0.0.1", 2300));
  EXPECT_EQ("*3\r\n$9\r\nreplicaof\r\n$9\r\n127.0.0.1\r\n$4\r\n2300\r\n",
            trpc::redis::cmdgen{}.replicaof("127.0.0.1", 2300));
  EXPECT_EQ("*2\r\n$7\r\nslowlog\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.slowlog("mykey"));
  EXPECT_EQ("*3\r\n$7\r\nslowlog\r\n$6\r\nmykey1\r\n$6\r\nmykey2\r\n",
            trpc::redis::cmdgen{}.slowlog("mykey1", "mykey2"));
  EXPECT_EQ("*1\r\n$4\r\nsync\r\n", trpc::redis::cmdgen{}.sync());
  EXPECT_EQ("*3\r\n$5\r\npsync\r\n$1\r\n1\r\n$3\r\n100\r\n", trpc::redis::cmdgen{}.psync(1, 100));
  EXPECT_EQ("*1\r\n$4\r\ntime\r\n", trpc::redis::cmdgen{}.time());
  EXPECT_EQ("*2\r\n$7\r\nLATENCY\r\n$6\r\nDOCTOR\r\n", trpc::redis::cmdgen{}.latency_doctor());
  EXPECT_EQ("*3\r\n$7\r\nLATENCY\r\n$5\r\nGRAPH\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.latency_graph("mykey"));
  EXPECT_EQ("*3\r\n$7\r\nLATENCY\r\n$7\r\nHISTORY\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.latency_history("mykey"));
  EXPECT_EQ("*2\r\n$7\r\nLATENCY\r\n$6\r\nLATEST\r\n", trpc::redis::cmdgen{}.latency_latest());
  EXPECT_EQ("*2\r\n$7\r\nLATENCY\r\n$5\r\nRESET\r\n", trpc::redis::cmdgen{}.latency_reset());
  EXPECT_EQ("*3\r\n$7\r\nLATENCY\r\n$5\r\nRESET\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.latency_reset("mykey"));
  EXPECT_EQ("*2\r\n$7\r\nLATENCY\r\n$4\r\nHELP\r\n", trpc::redis::cmdgen{}.latency_help());
}

TEST(CmdgenTest, sets) {
  std::vector<std::string> keys;
  keys.push_back("key1");
  keys.push_back("key2");
  keys.push_back("key3");
  EXPECT_EQ("*5\r\n$4\r\nsadd\r\n$5\r\nmykey\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.sadd("mykey", keys));
  EXPECT_EQ("*2\r\n$5\r\nscard\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.scard("mykey"));
  EXPECT_EQ("*4\r\n$5\r\nsdiff\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.sdiff(keys));
  EXPECT_EQ("*5\r\n$10\r\nsdiffstore\r\n$5\r\nmykey\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.sdiffstore("mykey", keys));
  EXPECT_EQ("*4\r\n$6\r\nsinter\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.sinter(keys));
  EXPECT_EQ("*5\r\n$11\r\nsinterstore\r\n$5\r\nmykey\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.sinterstore("mykey", keys));
  EXPECT_EQ("*3\r\n$9\r\nsismember\r\n$5\r\nmykey\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.sismember("mykey", "mymember"));
  EXPECT_EQ("*2\r\n$8\r\nsmembers\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.smembers("mykey"));
  EXPECT_EQ("*4\r\n$5\r\nsmove\r\n$5\r\nmykey\r\n$6\r\nmykey2\r\n$6\r\nmykey3\r\n",
            trpc::redis::cmdgen{}.smove("mykey", "mykey2", "mykey3"));
  EXPECT_EQ("*2\r\n$4\r\nspop\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.spop("mykey"));
  EXPECT_EQ("*3\r\n$4\r\nspop\r\n$5\r\nmykey\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.spop("mykey", 100));
  EXPECT_EQ("*2\r\n$11\r\nsrandmember\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.srandmember("mykey"));
  EXPECT_EQ("*5\r\n$4\r\nsrem\r\n$5\r\nmykey\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.srem("mykey", keys));
  EXPECT_EQ("*4\r\n$6\r\nsunion\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.sunion(keys));
  EXPECT_EQ("*5\r\n$11\r\nsunionstore\r\n$5\r\nmykey\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.sunionstore("mykey", keys));
  EXPECT_EQ("*3\r\n$5\r\nsscan\r\n$5\r\nmykey\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.sscan("mykey", 100));
  EXPECT_EQ("*5\r\n$5\r\nsscan\r\n$5\r\nmykey\r\n$3\r\n100\r\n$5\r\nMATCH\r\n$6\r\nmykey2\r\n",
            trpc::redis::cmdgen{}.sscan("mykey", 100, "mykey2"));
  EXPECT_EQ("*5\r\n$5\r\nsscan\r\n$5\r\nmykey\r\n$3\r\n100\r\n$5\r\nCOUNT\r\n$3\r\n200\r\n",
            trpc::redis::cmdgen{}.sscan("mykey", 100, 200));
  EXPECT_EQ(
      "*7\r\n$5\r\nsscan\r\n$5\r\nmykey\r\n$3\r\n100\r\n$5\r\nMATCH\r\n$6\r\nmykey2\r\n$"
      "5\r\nCOUNT\r\n$3\r\n200\r\n",
      trpc::redis::cmdgen{}.sscan("mykey", 100, "mykey2", 200));
}

TEST(CmdgenTest, sortedsets) {
  std::vector<std::string> keys;
  keys.push_back("key1");
  keys.push_back("key2");
  keys.push_back("key3");
  std::vector<size_t> weights;
  weights.push_back(100);
  weights.push_back(998);
  weights.push_back(335);
  std::multimap<std::string, std::string> score_members;
  score_members.insert(std::make_pair("score1", "test1"));
  score_members.insert(std::make_pair("score2", "test2"));
  score_members.insert(std::make_pair("score3", "test3"));
  score_members.insert(std::make_pair("score4", "test4"));
  score_members.insert(std::make_pair("score5", "test5"));
  EXPECT_EQ("*5\r\n$8\r\nbzpopmin\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.bzpopmin(keys, 100));
  EXPECT_EQ("*5\r\n$8\r\nbzpopmax\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.bzpopmax(keys, 100));
  EXPECT_EQ(
      "*15\r\n$4\r\nzadd\r\n$5\r\nmykey\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n$"
      "6\r\nscore1\r\n$5\r\ntest1\r\n$6\r\nscore2\r\n$5\r\ntest2\r\n$6\r\nscore3\r\n$"
      "5\r\ntest3\r\n$6\r\nscore4\r\n$5\r\ntest4\r\n$6\r\nscore5\r\n$5\r\ntest5\r\n",
      trpc::redis::cmdgen{}.zadd("mykey", keys, score_members));
  EXPECT_EQ("*2\r\n$5\r\nzcard\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.zcard("mykey"));
  EXPECT_EQ("*4\r\n$6\r\nzcount\r\n$5\r\nmykey\r\n$2\r\n10\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.zcount("mykey", 10, 100));
  EXPECT_EQ("*4\r\n$6\r\nzcount\r\n$5\r\nmykey\r\n$8\r\n1.000000\r\n$9\r\n20.900000\r\n",
            trpc::redis::cmdgen{}.zcount("mykey", 1.0, 20.9));
  EXPECT_EQ("*4\r\n$6\r\nzcount\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$3\r\nxms\r\n",
            trpc::redis::cmdgen{}.zcount("mykey", "abc", "xms"));
  EXPECT_EQ("*4\r\n$7\r\nzincrby\r\n$5\r\nmykey\r\n$1\r\n5\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zincrby("mykey", 5, "mymember"));
  EXPECT_EQ("*4\r\n$7\r\nzincrby\r\n$5\r\nmykey\r\n$9\r\n10.000000\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zincrby("mykey", 10.0, "mymember"));
  EXPECT_EQ("*4\r\n$7\r\nzincrby\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zincrby("mykey", "abc", "mymember"));
  EXPECT_EQ(
      "*12\r\n$11\r\nzinterstore\r\n$5\r\nmykey\r\n$3\r\n100\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$"
      "4\r\nkey3\r\n$7\r\nWEIGHTS\r\n$3\r\n100\r\n$3\r\n998\r\n$3\r\n335\r\n$9\r\nAGGREGATE\r\n$"
      "3\r\nSUM\r\n",
      trpc::redis::cmdgen{}.zinterstore("mykey", 100, keys, weights,
                                        trpc::redis::cmdgen::aggregate_method::sum));
  EXPECT_EQ("*4\r\n$9\r\nzlexcount\r\n$5\r\nmykey\r\n$3\r\n150\r\n$3\r\n260\r\n",
            trpc::redis::cmdgen{}.zlexcount("mykey", 150, 260));
  EXPECT_EQ("*4\r\n$9\r\nzlexcount\r\n$5\r\nmykey\r\n$8\r\n1.200000\r\n$8\r\n5.900000\r\n",
            trpc::redis::cmdgen{}.zlexcount("mykey", 1.2, 5.9));
  EXPECT_EQ("*4\r\n$9\r\nzlexcount\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zlexcount("mykey", "abc", "mymember"));
  EXPECT_EQ("*2\r\n$7\r\nzpopmax\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.zpopmax("mykey"));
  EXPECT_EQ("*3\r\n$7\r\nzpopmax\r\n$5\r\nmykey\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.zpopmax("mykey", 100));
  EXPECT_EQ("*2\r\n$7\r\nzpopmin\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.zpopmin("mykey"));
  EXPECT_EQ("*3\r\n$7\r\nzpopmin\r\n$5\r\nmykey\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.zpopmin("mykey", 100));
  EXPECT_EQ("*4\r\n$6\r\nzrange\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n",
            trpc::redis::cmdgen{}.zrange("mykey", 103, 205));
  EXPECT_EQ(
      "*5\r\n$6\r\nzrange\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n$10\r\nWITHSCORES\r\n",
      trpc::redis::cmdgen{}.zrange("mykey", "abc", "mymember", true));
  EXPECT_EQ("*4\r\n$6\r\nzrange\r\n$5\r\nmykey\r\n$8\r\n1.200000\r\n$8\r\n3.500000\r\n",
            trpc::redis::cmdgen{}.zrange("mykey", 1.2, 3.5));
  EXPECT_EQ(
      "*5\r\n$6\r\nzrange\r\n$5\r\nmykey\r\n$8\r\n1.200000\r\n$8\r\n3.500000\r\n$"
      "10\r\nWITHSCORES\r\n",
      trpc::redis::cmdgen{}.zrange("mykey", 1.2, 3.5, true));
  EXPECT_EQ("*4\r\n$6\r\nzrange\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zrange("mykey", "abc", "mymember"));
  EXPECT_EQ(
      "*5\r\n$6\r\nzrange\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n$10\r\nWITHSCORES\r\n",
      trpc::redis::cmdgen{}.zrange("mykey", "abc", "mymember", true));
  EXPECT_EQ("*4\r\n$11\r\nzrangebylex\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n",
            trpc::redis::cmdgen{}.zrangebylex("mykey", 103, 205));
  EXPECT_EQ("*4\r\n$11\r\nzrangebylex\r\n$5\r\nmykey\r\n$8\r\n1.200000\r\n$8\r\n3.900000\r\n",
            trpc::redis::cmdgen{}.zrangebylex("mykey", 1.2, 3.9));
  EXPECT_EQ("*4\r\n$11\r\nzrangebylex\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zrangebylex("mykey", "abc", "mymember"));
  EXPECT_EQ(
      "*7\r\n$11\r\nzrangebylex\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n$5\r\nLIMIT\r\n$"
      "2\r\n10\r\n$2\r\n35\r\n",
      trpc::redis::cmdgen{}.zrangebylex("mykey", 103, 205, 10, 35));
  EXPECT_EQ(
      "*7\r\n$11\r\nzrangebylex\r\n$5\r\nmykey\r\n$8\r\n1.200000\r\n$8\r\n3.900000\r\n$"
      "5\r\nLIMIT\r\n$2\r\n10\r\n$2\r\n35\r\n",
      trpc::redis::cmdgen{}.zrangebylex("mykey", 1.2, 3.9, 10, 35));
  EXPECT_EQ(
      "*7\r\n$11\r\nzrangebylex\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n$5\r\nLIMIT\r\n$"
      "3\r\n103\r\n$3\r\n205\r\n",
      trpc::redis::cmdgen{}.zrangebylex("mykey", "abc", "mymember", 103, 205));
  EXPECT_EQ("*4\r\n$14\r\nzrevrangebylex\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n",
            trpc::redis::cmdgen{}.zrevrangebylex("mykey", 103, 205));
  EXPECT_EQ("*4\r\n$14\r\nzrevrangebylex\r\n$5\r\nmykey\r\n$8\r\n1.200000\r\n$8\r\n3.900000\r\n",
            trpc::redis::cmdgen{}.zrevrangebylex("mykey", 1.2, 3.9));
  EXPECT_EQ("*4\r\n$14\r\nzrevrangebylex\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zrevrangebylex("mykey", "abc", "mymember"));
  EXPECT_EQ(
      "*7\r\n$14\r\nzrevrangebylex\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n$5\r\nLIMIT\r\n$"
      "3\r\n100\r\n$2\r\n20\r\n",
      trpc::redis::cmdgen{}.zrevrangebylex("mykey", 103, 205, 100, 20));
  EXPECT_EQ(
      "*7\r\n$14\r\nzrevrangebylex\r\n$5\r\nmykey\r\n$8\r\n1.200000\r\n$8\r\n3.900000\r\n$"
      "5\r\nLIMIT\r\n$3\r\n100\r\n$2\r\n20\r\n",
      trpc::redis::cmdgen{}.zrevrangebylex("mykey", 1.2, 3.9, 100, 20));
  EXPECT_EQ(
      "*7\r\n$14\r\nzrevrangebylex\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n$"
      "5\r\nLIMIT\r\n$3\r\n103\r\n$3\r\n205\r\n",
      trpc::redis::cmdgen{}.zrevrangebylex("mykey", "abc", "mymember", 103, 205));
  EXPECT_EQ("*4\r\n$13\r\nzrangebyscore\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n",
            trpc::redis::cmdgen{}.zrangebyscore("mykey", 103, 205));
  EXPECT_EQ(
      "*5\r\n$13\r\nzrangebyscore\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n$"
      "10\r\nWITHSCORES\r\n",
      trpc::redis::cmdgen{}.zrangebyscore("mykey", 103, 205, true));
  EXPECT_EQ("*4\r\n$13\r\nzrangebyscore\r\n$5\r\nmykey\r\n$8\r\n1.900000\r\n$8\r\n5.010000\r\n",
            trpc::redis::cmdgen{}.zrangebyscore("mykey", 1.9, 5.01));
  EXPECT_EQ(
      "*5\r\n$13\r\nzrangebyscore\r\n$5\r\nmykey\r\n$8\r\n1.900000\r\n$8\r\n5.010000\r\n$"
      "10\r\nWITHSCORES\r\n",
      trpc::redis::cmdgen{}.zrangebyscore("mykey", 1.9, 5.01, true));
  EXPECT_EQ("*4\r\n$13\r\nzrangebyscore\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zrangebyscore("mykey", "abc", "mymember"));
  EXPECT_EQ(
      "*5\r\n$13\r\nzrangebyscore\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n$"
      "10\r\nWITHSCORES\r\n",
      trpc::redis::cmdgen{}.zrangebyscore("mykey", "abc", "mymember", true));
  EXPECT_EQ(
      "*7\r\n$13\r\nzrangebyscore\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n$5\r\nLIMIT\r\n$"
      "3\r\n100\r\n$2\r\n20\r\n",
      trpc::redis::cmdgen{}.zrangebyscore("mykey", 103, 205, 100, 20));
  EXPECT_EQ(
      "*8\r\n$13\r\nzrangebyscore\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n$"
      "10\r\nWITHSCORES\r\n$5\r\nLIMIT\r\n$3\r\n100\r\n$2\r\n20\r\n",
      trpc::redis::cmdgen{}.zrangebyscore("mykey", 103, 205, 100, 20, true));
  EXPECT_EQ(
      "*7\r\n$13\r\nzrangebyscore\r\n$5\r\nmykey\r\n$8\r\n1.900000\r\n$8\r\n5.010000\r\n$"
      "5\r\nLIMIT\r\n$3\r\n100\r\n$2\r\n20\r\n",
      trpc::redis::cmdgen{}.zrangebyscore("mykey", 1.9, 5.01, 100, 20));
  EXPECT_EQ(
      "*8\r\n$13\r\nzrangebyscore\r\n$5\r\nmykey\r\n$8\r\n1.900000\r\n$8\r\n5.010000\r\n$"
      "10\r\nWITHSCORES\r\n$5\r\nLIMIT\r\n$3\r\n100\r\n$2\r\n20\r\n",
      trpc::redis::cmdgen{}.zrangebyscore("mykey", 1.9, 5.01, 100, 20, true));
  EXPECT_EQ(
      "*7\r\n$13\r\nzrangebyscore\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n$5\r\nLIMIT\r\n$"
      "3\r\n100\r\n$2\r\n20\r\n",
      trpc::redis::cmdgen{}.zrangebyscore("mykey", "abc", "mymember", 100, 20));
  EXPECT_EQ(
      "*8\r\n$13\r\nzrangebyscore\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n$"
      "10\r\nWITHSCORES\r\n$5\r\nLIMIT\r\n$3\r\n100\r\n$2\r\n20\r\n",
      trpc::redis::cmdgen{}.zrangebyscore("mykey", "abc", "mymember", 100, 20, true));
  EXPECT_EQ("*3\r\n$5\r\nzrank\r\n$5\r\nmykey\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zrank("mykey", "mymember"));
  EXPECT_EQ("*5\r\n$4\r\nzrem\r\n$5\r\nmykey\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.zrem("mykey", keys));
  EXPECT_EQ("*4\r\n$14\r\nzremrangebylex\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n",
            trpc::redis::cmdgen{}.zremrangebylex("mykey", 103, 205));
  EXPECT_EQ("*4\r\n$14\r\nzremrangebylex\r\n$5\r\nmykey\r\n$8\r\n1.900000\r\n$8\r\n9.010100\r\n",
            trpc::redis::cmdgen{}.zremrangebylex("mykey", 1.9, 9.0101));
  EXPECT_EQ("*4\r\n$14\r\nzremrangebylex\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zremrangebylex("mykey", "abc", "mymember"));
  EXPECT_EQ("*4\r\n$15\r\nzremrangebyrank\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n",
            trpc::redis::cmdgen{}.zremrangebyrank("mykey", 103, 205));
  EXPECT_EQ("*4\r\n$15\r\nzremrangebyrank\r\n$5\r\nmykey\r\n$8\r\n1.900000\r\n$8\r\n9.010100\r\n",
            trpc::redis::cmdgen{}.zremrangebyrank("mykey", 1.9, 9.0101));
  EXPECT_EQ("*4\r\n$15\r\nzremrangebyrank\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zremrangebyrank("mykey", "abc", "mymember"));
  EXPECT_EQ("*4\r\n$16\r\nzremrangebyscore\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n",
            trpc::redis::cmdgen{}.zremrangebyscore("mykey", 103, 205));
  EXPECT_EQ("*4\r\n$16\r\nzremrangebyscore\r\n$5\r\nmykey\r\n$8\r\n1.900000\r\n$8\r\n9.010100\r\n",
            trpc::redis::cmdgen{}.zremrangebyscore("mykey", 1.9, 9.0101));
  EXPECT_EQ("*4\r\n$16\r\nzremrangebyscore\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zremrangebyscore("mykey", "abc", "mymember"));
  EXPECT_EQ("*4\r\n$9\r\nzrevrange\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n",
            trpc::redis::cmdgen{}.zrevrange("mykey", 103, 205));
  EXPECT_EQ(
      "*5\r\n$9\r\nzrevrange\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n$10\r\nWITHSCORES\r\n",
      trpc::redis::cmdgen{}.zrevrange("mykey", 103, 205, true));
  EXPECT_EQ("*4\r\n$9\r\nzrevrange\r\n$5\r\nmykey\r\n$8\r\n1.900000\r\n$8\r\n9.010100\r\n",
            trpc::redis::cmdgen{}.zrevrange("mykey", 1.9, 9.0101));
  EXPECT_EQ(
      "*5\r\n$9\r\nzrevrange\r\n$5\r\nmykey\r\n$8\r\n1.900000\r\n$8\r\n9.010100\r\n$"
      "10\r\nWITHSCORES\r\n",
      trpc::redis::cmdgen{}.zrevrange("mykey", 1.9, 9.0101, true));
  EXPECT_EQ("*4\r\n$9\r\nzrevrange\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zrevrange("mykey", "abc", "mymember"));
  EXPECT_EQ(
      "*5\r\n$9\r\nzrevrange\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n$"
      "10\r\nWITHSCORES\r\n",
      trpc::redis::cmdgen{}.zrevrange("mykey", "abc", "mymember", true));
  EXPECT_EQ("*4\r\n$16\r\nzrevrangebyscore\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n",
            trpc::redis::cmdgen{}.zrevrangebyscore("mykey", 103, 205));
  EXPECT_EQ(
      "*5\r\n$16\r\nzrevrangebyscore\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n$"
      "10\r\nWITHSCORES\r\n",
      trpc::redis::cmdgen{}.zrevrangebyscore("mykey", 103, 205, true));
  EXPECT_EQ("*4\r\n$16\r\nzrevrangebyscore\r\n$5\r\nmykey\r\n$8\r\n1.900000\r\n$8\r\n9.010100\r\n",
            trpc::redis::cmdgen{}.zrevrangebyscore("mykey", 1.9, 9.0101));
  EXPECT_EQ(
      "*5\r\n$16\r\nzrevrangebyscore\r\n$5\r\nmykey\r\n$8\r\n1.900000\r\n$8\r\n9.010100\r\n$"
      "10\r\nWITHSCORES\r\n",
      trpc::redis::cmdgen{}.zrevrangebyscore("mykey", 1.9, 9.0101, true));
  EXPECT_EQ("*4\r\n$16\r\nzrevrangebyscore\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zrevrangebyscore("mykey", "abc", "mymember"));
  EXPECT_EQ(
      "*5\r\n$16\r\nzrevrangebyscore\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n$"
      "10\r\nWITHSCORES\r\n",
      trpc::redis::cmdgen{}.zrevrangebyscore("mykey", "abc", "mymember", true));
  EXPECT_EQ(
      "*7\r\n$16\r\nzrevrangebyscore\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n$5\r\nLIMIT\r\n$"
      "3\r\n150\r\n$3\r\n300\r\n",
      trpc::redis::cmdgen{}.zrevrangebyscore("mykey", 103, 205, 150, 300));
  EXPECT_EQ(
      "*8\r\n$16\r\nzrevrangebyscore\r\n$5\r\nmykey\r\n$3\r\n103\r\n$3\r\n205\r\n$"
      "10\r\nWITHSCORES\r\n$5\r\nLIMIT\r\n$3\r\n150\r\n$3\r\n300\r\n",
      trpc::redis::cmdgen{}.zrevrangebyscore("mykey", 103, 205, 150, 300, true));
  EXPECT_EQ(
      "*7\r\n$16\r\nzrevrangebyscore\r\n$5\r\nmykey\r\n$8\r\n1.900000\r\n$8\r\n9.010100\r\n$"
      "5\r\nLIMIT\r\n$3\r\n150\r\n$3\r\n300\r\n",
      trpc::redis::cmdgen{}.zrevrangebyscore("mykey", 1.9, 9.0101, 150, 300));
  EXPECT_EQ(
      "*8\r\n$16\r\nzrevrangebyscore\r\n$5\r\nmykey\r\n$8\r\n1.900000\r\n$8\r\n9.010100\r\n$"
      "10\r\nWITHSCORES\r\n$5\r\nLIMIT\r\n$3\r\n150\r\n$3\r\n300\r\n",
      trpc::redis::cmdgen{}.zrevrangebyscore("mykey", 1.9, 9.0101, 150, 300, true));
  EXPECT_EQ(
      "*7\r\n$16\r\nzrevrangebyscore\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n$"
      "5\r\nLIMIT\r\n$3\r\n150\r\n$3\r\n300\r\n",
      trpc::redis::cmdgen{}.zrevrangebyscore("mykey", "abc", "mymember", 150, 300));
  EXPECT_EQ(
      "*8\r\n$16\r\nzrevrangebyscore\r\n$5\r\nmykey\r\n$3\r\nabc\r\n$8\r\nmymember\r\n$"
      "10\r\nWITHSCORES\r\n$5\r\nLIMIT\r\n$3\r\n150\r\n$3\r\n300\r\n",
      trpc::redis::cmdgen{}.zrevrangebyscore("mykey", "abc", "mymember", 150, 300, true));
  EXPECT_EQ("*3\r\n$8\r\nzrevrank\r\n$5\r\nmykey\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zrevrank("mykey", "mymember"));
  EXPECT_EQ("*3\r\n$6\r\nzscore\r\n$5\r\nmykey\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zscore("mykey", "mymember"));
  EXPECT_EQ(
      "*12\r\n$11\r\nzunionstore\r\n$5\r\nmykey\r\n$3\r\n100\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$"
      "4\r\nkey3\r\n$7\r\nWEIGHTS\r\n$3\r\n100\r\n$3\r\n998\r\n$3\r\n335\r\n$9\r\nAGGREGATE\r\n$"
      "3\r\nMIN\r\n",
      trpc::redis::cmdgen{}.zunionstore("mykey", 100, keys, weights,
                                        trpc::redis::cmdgen::aggregate_method::min));
  EXPECT_EQ("*3\r\n$5\r\nzscan\r\n$5\r\nmykey\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.zscan("mykey", 100));
  EXPECT_EQ("*5\r\n$5\r\nzscan\r\n$5\r\nmykey\r\n$3\r\n100\r\n$5\r\nMATCH\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zscan("mykey", 100, "mymember"));
  EXPECT_EQ("*5\r\n$5\r\nzscan\r\n$5\r\nmykey\r\n$3\r\n150\r\n$5\r\nCOUNT\r\n$3\r\n300\r\n",
            trpc::redis::cmdgen{}.zscan("mykey", 150, 300));
  EXPECT_EQ("*3\r\n$5\r\nzscan\r\n$5\r\nmykey\r\n$3\r\n150\r\n",
            trpc::redis::cmdgen{}.zscan("mykey", 150, "", 0));
  EXPECT_EQ("*5\r\n$5\r\nzscan\r\n$5\r\nmykey\r\n$3\r\n150\r\n$5\r\nMATCH\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.zscan("mykey", 150, "mymember", 0));
  EXPECT_EQ("*5\r\n$5\r\nzscan\r\n$5\r\nmykey\r\n$3\r\n150\r\n$5\r\nCOUNT\r\n$3\r\n112\r\n",
            trpc::redis::cmdgen{}.zscan("mykey", 150, "", 112));
  EXPECT_EQ(
      "*7\r\n$5\r\nzscan\r\n$5\r\nmykey\r\n$3\r\n150\r\n$5\r\nMATCH\r\n$8\r\nmymember\r\n$"
      "5\r\nCOUNT\r\n$3\r\n112\r\n",
      trpc::redis::cmdgen{}.zscan("mykey", 150, "mymember", 112));
}

TEST(CmdgenTest, strings) {
  std::vector<trpc::redis::cmdgen::bitfield_operation> operations;
  operations.push_back(trpc::redis::cmdgen::bitfield_operation::get("int", 100));
  operations.push_back(trpc::redis::cmdgen::bitfield_operation::get(
      "string", 100, trpc::redis::cmdgen::overflow_type::wrap));
  operations.push_back(trpc::redis::cmdgen::bitfield_operation::set(
      "list", 10, 100, trpc::redis::cmdgen::overflow_type::sat));
  operations.push_back(trpc::redis::cmdgen::bitfield_operation::incrby(
      "sets", 24, 100, trpc::redis::cmdgen::overflow_type::fail));

  std::vector<std::string> keys;
  keys.push_back("key1");
  keys.push_back("key2");
  keys.push_back("key3");

  std::vector<std::pair<std::string, std::string>> key_values;
  key_values.push_back(std::make_pair("key1", "value1"));
  key_values.push_back(std::make_pair("key2", "value2"));
  key_values.push_back(std::make_pair("key3", "value3"));
  key_values.push_back(std::make_pair("key4", "value4"));
  key_values.push_back(std::make_pair("key5", "value5"));

  EXPECT_EQ("*3\r\n$6\r\nappend\r\n$5\r\nmykey\r\n$8\r\nmymember\r\n",
            trpc::redis::cmdgen{}.append("mykey", "mymember"));
  EXPECT_EQ("*2\r\n$8\r\nbitcount\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.bitcount("mykey"));
  EXPECT_EQ("*4\r\n$8\r\nbitcount\r\n$5\r\nmykey\r\n$1\r\n2\r\n$1\r\n5\r\n",
            trpc::redis::cmdgen{}.bitcount("mykey", 2, 5));
  EXPECT_EQ(
      "*22\r\n$8\r\nbitfield\r\n$5\r\nmykey\r\n$3\r\nGET\r\n$3\r\nint\r\n$3\r\n100\r\n$"
      "3\r\nGET\r\n$6\r\nstring\r\n$3\r\n100\r\n$8\r\nOVERFLOW\r\n$4\r\nWRAP\r\n$3\r\nSET\r\n$"
      "4\r\nlist\r\n$2\r\n10\r\n$3\r\n100\r\n$8\r\nOVERFLOW\r\n$3\r\nSAT\r\n$6\r\nINCRBY\r\n$"
      "4\r\nsets\r\n$2\r\n24\r\n$3\r\n100\r\n$8\r\nOVERFLOW\r\n$4\r\nFAIL\r\n",
      trpc::redis::cmdgen{}.bitfield("mykey", operations));
  EXPECT_EQ(
      "*6\r\n$5\r\nbitop\r\n$5\r\nmykey\r\n$8\r\nmymember\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$"
      "4\r\nkey3\r\n",
      trpc::redis::cmdgen{}.bitop("mykey", "mymember", keys));
  EXPECT_EQ("*3\r\n$6\r\nbitpos\r\n$5\r\nmykey\r\n$2\r\n10\r\n",
            trpc::redis::cmdgen{}.bitpos("mykey", 10));
  EXPECT_EQ("*4\r\n$6\r\nbitpos\r\n$5\r\nmykey\r\n$2\r\n10\r\n$2\r\n30\r\n",
            trpc::redis::cmdgen{}.bitpos("mykey", 10, 30));
  EXPECT_EQ("*5\r\n$6\r\nbitpos\r\n$5\r\nmykey\r\n$2\r\n10\r\n$2\r\n20\r\n$2\r\n50\r\n",
            trpc::redis::cmdgen{}.bitpos("mykey", 10, 20, 50));
  EXPECT_EQ("*2\r\n$4\r\ndecr\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.decr("mykey"));
  EXPECT_EQ("*3\r\n$6\r\ndecrby\r\n$5\r\nmykey\r\n$2\r\n10\r\n",
            trpc::redis::cmdgen{}.decrby("mykey", 10));
  EXPECT_EQ("*2\r\n$3\r\nget\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.get("mykey"));
  EXPECT_EQ("*3\r\n$6\r\ngetbit\r\n$5\r\nmykey\r\n$2\r\n10\r\n",
            trpc::redis::cmdgen{}.getbit("mykey", 10));
  EXPECT_EQ("*4\r\n$8\r\ngetrange\r\n$5\r\nmykey\r\n$2\r\n10\r\n$2\r\n20\r\n",
            trpc::redis::cmdgen{}.getrange("mykey", 10, 20));
  EXPECT_EQ("*3\r\n$6\r\ngetset\r\n$5\r\nmykey\r\n$6\r\nmember\r\n",
            trpc::redis::cmdgen{}.getset("mykey", "member"));
  EXPECT_EQ("*2\r\n$4\r\nincr\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.incr("mykey"));
  EXPECT_EQ("*3\r\n$6\r\nincrby\r\n$5\r\nmykey\r\n$3\r\n100\r\n",
            trpc::redis::cmdgen{}.incrby("mykey", 100));
  EXPECT_EQ("*3\r\n$11\r\nincrbyfloat\r\n$5\r\nmykey\r\n$8\r\n1.200000\r\n",
            trpc::redis::cmdgen{}.incrbyfloat("mykey", 1.2f));
  EXPECT_EQ("*4\r\n$4\r\nmget\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.mget(keys));
  EXPECT_EQ(
      "*11\r\n$4\r\nmset\r\n$4\r\nkey1\r\n$6\r\nvalue1\r\n$4\r\nkey2\r\n$6\r\nvalue2\r\n$"
      "4\r\nkey3\r\n$6\r\nvalue3\r\n$4\r\nkey4\r\n$6\r\nvalue4\r\n$4\r\nkey5\r\n$6\r\nvalue5\r\n",
      trpc::redis::cmdgen{}.mset(key_values));
  EXPECT_EQ(
      "*11\r\n$6\r\nmsetnx\r\n$4\r\nkey1\r\n$6\r\nvalue1\r\n$4\r\nkey2\r\n$6\r\nvalue2\r\n$"
      "4\r\nkey3\r\n$6\r\nvalue3\r\n$4\r\nkey4\r\n$6\r\nvalue4\r\n$4\r\nkey5\r\n$6\r\nvalue5\r\n",
      trpc::redis::cmdgen{}.msetnx(key_values));
  EXPECT_EQ("*4\r\n$6\r\npsetex\r\n$5\r\nmykey\r\n$5\r\n10000\r\n$5\r\nvalue\r\n",
            trpc::redis::cmdgen{}.psetex("mykey", 10000, "value"));
  EXPECT_EQ("*3\r\n$3\r\nset\r\n$5\r\nmykey\r\n$5\r\nvalue\r\n",
            trpc::redis::cmdgen{}.set("mykey", "value"));
  EXPECT_EQ("*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$5\r\nvalue\r\n",
            trpc::redis::cmdgen{}.set("mykey", "value", false, 5, false, 100, false, false));
  EXPECT_EQ(
      "*9\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$5\r\nvalue\r\n$2\r\nEX\r\n$1\r\n5\r\n$2\r\nPX\r\n$"
      "3\r\n100\r\n$2\r\nNX\r\n$2\r\nXX\r\n",
      trpc::redis::cmdgen{}.set("mykey", "value", true, 5, true, 100, true, true));
  EXPECT_EQ("*4\r\n$6\r\nsetbit\r\n$5\r\nmykey\r\n$1\r\n2\r\n$1\r\n1\r\n",
            trpc::redis::cmdgen{}.__setbit("mykey", 2, "1"));
  EXPECT_EQ("*4\r\n$5\r\nsetex\r\n$5\r\nmykey\r\n$3\r\n100\r\n$5\r\nvalue\r\n",
            trpc::redis::cmdgen{}.setex("mykey", 100, "value"));
  EXPECT_EQ("*3\r\n$5\r\nsetnx\r\n$5\r\nmykey\r\n$5\r\nvalue\r\n",
            trpc::redis::cmdgen{}.setnx("mykey", "value"));
  EXPECT_EQ("*4\r\n$8\r\nsetrange\r\n$5\r\nmykey\r\n$1\r\n2\r\n$5\r\nvalue\r\n",
            trpc::redis::cmdgen{}.setrange("mykey", 2, "value"));
  EXPECT_EQ("*2\r\n$6\r\nstrlen\r\n$5\r\nmykey\r\n", trpc::redis::cmdgen{}.strlen("mykey"));
}

TEST(CmdgenTest, transaction) {
  std::vector<std::string> keys;
  keys.push_back("key1");
  keys.push_back("key2");
  keys.push_back("key3");
  EXPECT_EQ("*1\r\n$7\r\ndiscard\r\n", trpc::redis::cmdgen{}.discard());
  EXPECT_EQ("*1\r\n$4\r\nexec\r\n", trpc::redis::cmdgen{}.exec());
  EXPECT_EQ("*1\r\n$5\r\nmulti\r\n", trpc::redis::cmdgen{}.multi());
  EXPECT_EQ("*1\r\n$7\r\nunwatch\r\n", trpc::redis::cmdgen{}.unwatch());
  EXPECT_EQ("*4\r\n$5\r\nwatch\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n$4\r\nkey3\r\n",
            trpc::redis::cmdgen{}.watch(keys));
  EXPECT_EQ("*3\r\n$8\r\nSENTINEL\r\n$8\r\nCKQUORUM\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.sentinel_ckquorum("mykey"));
  EXPECT_EQ("*3\r\n$8\r\nSENTINEL\r\n$8\r\nFAILOVER\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.sentinel_failover("mykey"));
  EXPECT_EQ("*2\r\n$8\r\nSENTINEL\r\n$11\r\nFLUSHCONFIG\r\n",
            trpc::redis::cmdgen{}.sentinel_flushconfig());
  EXPECT_EQ("*3\r\n$8\r\nSENTINEL\r\n$6\r\nMASTER\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.sentinel_master("mykey"));
  EXPECT_EQ("*2\r\n$8\r\nSENTINEL\r\n$7\r\nMASTERS\r\n", trpc::redis::cmdgen{}.sentinel_masters());
  EXPECT_EQ(
      "*6\r\n$8\r\nSENTINEL\r\n$7\r\nMONITOR\r\n$5\r\nmykey\r\n$9\r\n127.0.0.1\r\n$4\r\n6554\r\n$"
      "3\r\n235\r\n",
      trpc::redis::cmdgen{}.sentinel_monitor("mykey", "127.0.0.1", 6554, 235));
  EXPECT_EQ("*2\r\n$8\r\nSENTINEL\r\n$4\r\nPING\r\n", trpc::redis::cmdgen{}.sentinel_ping());
  EXPECT_EQ("*3\r\n$8\r\nSENTINEL\r\n$6\r\nREMOVE\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.sentinel_remove("mykey"));
  EXPECT_EQ("*3\r\n$8\r\nSENTINEL\r\n$5\r\nRESET\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.sentinel_reset("mykey"));
  EXPECT_EQ("*3\r\n$8\r\nSENTINEL\r\n$9\r\nSENTINELS\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.sentinel_sentinels("mykey"));
  EXPECT_EQ("*5\r\n$8\r\nSENTINEL\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$6\r\noption\r\n$5\r\nvalue\r\n",
            trpc::redis::cmdgen{}.sentinel_set("mykey", "option", "value"));
  EXPECT_EQ("*3\r\n$8\r\nSENTINEL\r\n$6\r\nSLAVES\r\n$5\r\nmykey\r\n",
            trpc::redis::cmdgen{}.sentinel_slaves("mykey"));
}

TEST(CmdgenTest, bitoperator) {
  EXPECT_EQ("*4\r\n$6\r\nsetbit\r\n$5\r\nmykey\r\n$5\r\n10001\r\n$1\r\n1\r\n",
            trpc::redis::cmdgen{}.__setbit("mykey", 10001, "1"));
}

TEST(CmdgenTest, bloomfilter) {
  EXPECT_EQ("*2\r\n$7\r\nbf.info\r\n$9\r\nuser_name\r\n",
            trpc::redis::cmdgen{}.bloom_info("user_name"));
  EXPECT_EQ("*4\r\n$10\r\nbf.reserve\r\n$9\r\nuser_name\r\n$8\r\n0.010000\r\n$4\r\n1000\r\n",
            trpc::redis::cmdgen{}.bloom_reserve("user_name", 0.01, 1000));
  EXPECT_EQ(
      "*7\r\n$10\r\nbf.reserve\r\n$9\r\nuser_name\r\n$8\r\n0.010000\r\n$4\r\n1000\r\n$"
      "9\r\nexpansion\r\n$1\r\n2\r\n$10\r\nnonscaling\r\n",
      trpc::redis::cmdgen{}.bloom_reserve("user_name", 0.01, 1000, 2, true));
  EXPECT_EQ("*3\r\n$6\r\nbf.add\r\n$9\r\nuser_name\r\n$5\r\nuser1\r\n",
            trpc::redis::cmdgen{}.bloom_add("user_name", "user1"));
  std::vector<std::string> users;
  users.emplace_back("user1");
  users.emplace_back("user2");
  users.emplace_back("user3");
  EXPECT_EQ(
      "*5\r\n$7\r\nbf.madd\r\n$9\r\nuser_name\r\n$5\r\nuser1\r\n$5\r\nuser2\r\n$5\r\nuser3\r\n",
      trpc::redis::cmdgen{}.bloom_madd("user_name", users));
  std::map<std::string, std::string> options;
  EXPECT_EQ(
      "*6\r\n$9\r\nbf.insert\r\n$9\r\nuser_name\r\n$5\r\nitems\r\n$5\r\nuser1\r\n$5\r\nuser2\r\n$"
      "5\r\nuser3\r\n",
      trpc::redis::cmdgen{}.bloom_insert("user_name", users, options));
  options["CAPACITY"] = "10000";
  options["ERROR"] = "0.01";
  options["EXPANSION"] = "2";
  EXPECT_EQ(
      "*14\r\n$9\r\nbf.insert\r\n$9\r\nuser_name\r\n$8\r\nCAPACITY\r\n$5\r\n10000\r\n$"
      "5\r\nERROR\r\n$4\r\n0.01\r\n$9\r\nEXPANSION\r\n$1\r\n2\r\n$8\r\nnocreate\r\n$"
      "10\r\nnonscaling\r\n$5\r\nitems\r\n$5\r\nuser1\r\n$5\r\nuser2\r\n$5\r\nuser3\r\n",
      trpc::redis::cmdgen{}.bloom_insert("user_name", users, options, true, true));
  EXPECT_EQ("*3\r\n$9\r\nbf.exists\r\n$9\r\nuser_name\r\n$5\r\nuser1\r\n",
            trpc::redis::cmdgen{}.bloom_exists("user_name", "user1"));
  EXPECT_EQ(
      "*5\r\n$10\r\nbf.mexists\r\n$9\r\nuser_name\r\n$5\r\nuser1\r\n$5\r\nuser2\r\n$5\r\nuser3\r\n",
      trpc::redis::cmdgen{}.bloom_mexists("user_name", users));
  EXPECT_EQ("*3\r\n$11\r\nbf.scandump\r\n$9\r\nuser_name\r\n$1\r\n0\r\n",
            trpc::redis::cmdgen{}.bloom_scandump("user_name", 0));
  EXPECT_EQ("*4\r\n$12\r\nbf.loadchunk\r\n$9\r\nuser_name\r\n$1\r\n0\r\n$8\r\ntestdata\r\n",
            trpc::redis::cmdgen{}.bloom_load_chunk("user_name", 0, "testdata"));
}

TEST(CmdgenTest, overflow) {
  EXPECT_EQ("*3\r\n$7\r\npexpire\r\n$5\r\nmykey\r\n$14\r\n15000000000000\r\n",
            trpc::redis::cmdgen{}.pexpire("mykey", 15000000000000));
  EXPECT_EQ("*3\r\n$9\r\npexpireat\r\n$5\r\nmykey\r\n$14\r\n15000000000000\r\n",
            trpc::redis::cmdgen{}.pexpireat("mykey", 15000000000000));
  EXPECT_EQ(
      "*9\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$5\r\nvalue\r\n$2\r\nEX\r\n$1\r\n5\r\n$2\r\nPX\r\n$"
      "14\r\n15000000000000\r\n$2\r\nNX\r\n$2\r\nXX\r\n",
      trpc::redis::cmdgen{}.set("mykey", "value", true, 5, true, 15000000000000, true, true));

  EXPECT_EQ("*4\r\n$6\r\npsetex\r\n$5\r\nmykey\r\n$14\r\n15000000000000\r\n$5\r\nvalue\r\n",
            trpc::redis::cmdgen{}.psetex("mykey", 15000000000000, "value"));
}

}  // namespace testing

}  // namespace trpc
