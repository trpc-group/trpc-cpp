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

#include "trpc/transport/server/default/connection_manager.h"

#include <memory>

#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/default/reactor_impl.h"
#include "trpc/runtime/iomodel/reactor/default/tcp_connection.h"

namespace trpc::testing {

TEST(ConnectionManagerTest, IdGenerator) {
  std::vector<uint32_t> max_conn_nums = {10, 64};
  for (auto max_conn_num : max_conn_nums) {
    ConnectionManager::IdGenerator id_gen(max_conn_num);
    uint32_t id;
    for (int i = 0; i < 10000; i++) {
      bool ret = id_gen.Get(id);
      ASSERT_TRUE(ret);
      uint64_t uid = (0x00000000FFFFFFFF & id);
      ASSERT_TRUE((uid & (id_gen.GetBaseNum() - 1)) <= max_conn_num);
      id_gen.Recycle(uid);
    }
  }
}

TEST(ConnectionManagerTest, Connection) {
  ConnectionManager conn_mgr(0, 1);

  uint64_t conn_id = conn_mgr.GenConnectionId();
  ASSERT_EQ(1, conn_id);

  ASSERT_EQ(0, conn_mgr.GenConnectionId());

  ASSERT_TRUE(nullptr == conn_mgr.GetConnection(conn_id));

  ReactorImpl::Options reactor_options;
  reactor_options.id = 0;
  std::unique_ptr<Reactor> reactor = std::make_unique<ReactorImpl>(reactor_options);

  RefPtr<TcpConnection> conn = MakeRefCounted<TcpConnection>(reactor.get(), Socket::CreateTcpSocket(false));
  conn->SetConnId(conn_id);

  conn_mgr.AddConnection(conn);
  ASSERT_TRUE(nullptr != conn_mgr.GetConnection(conn_id));

  RefPtr<TcpConnection> del_conn = conn_mgr.DelConnection(conn_id);
  ASSERT_TRUE(nullptr == conn_mgr.GetConnection(conn_id));
  conn_mgr.Stop();
  conn_mgr.Destroy();
}

}  // namespace trpc::testing
