//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/server/testing/trpc_client_testing.h"

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/codec/trpc/trpc_proto_checker.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/stream/trpc/testing/trpc_stream_testing.h"
#include "trpc/util/unique_id.h"

namespace trpc::testing {

using namespace std::chrono_literals;

uint32_t NextStreamId() {
  static trpc::UniqueId stream_id_generator{100};
  return stream_id_generator.GenerateId();
}

TestTrpcClientStreamProvider::TestTrpcClientStreamProvider(const Options& options) : options_(options) {
  conn_ = MakeRefCounted<trpc::testing::MockConnection>();
  conn_->SetConnType(ConnectionType::kTcpLong);
}

bool TestTrpcClientStreamProvider::SendAndRecvInit() {
  TrpcStreamInitMeta init_metadata{};
  init_metadata.set_content_type(TrpcContentEncodeType::TRPC_PROTO_ENCODE);
  init_metadata.set_content_encoding(TrpcCompressType::TRPC_DEFAULT_COMPRESS);

  TrpcStreamInitRequestMeta* request_meta = init_metadata.mutable_request_meta();
  request_meta->set_caller(options_.caller);
  request_meta->set_callee(options_.callee);
  request_meta->set_func(options_.func);

  TrpcStreamInitFrameProtocol init_frame = CreateTrpcStreamInitFrameProtocol(options_.stream_id, init_metadata);
  trpc::NoncontiguousBuffer send_buff;
  if (!init_frame.ZeroCopyEncode(send_buff)) return false;

  if (options_.transport->Send(std::move(send_buff)) <= 0) return false;
  std::cout << "send init frame, stream id: " << options_.stream_id << std::endl;

  trpc::stream::ProtocolMessageMetadata meta{};
  NoncontiguousBuffer init_frame_msg;
  if (!Recv(meta, init_frame_msg)) return false;
  std::cout << "recv init frame, stream id: " << options_.stream_id << std::endl;

  bool init_ok = (meta.stream_frame_type == static_cast<uint8_t>(trpc::stream::StreamMessageCategory::kStreamInit));
  if (!init_ok) return false;

  TrpcStreamInitFrameProtocol rsp_init_frame;
  if (!rsp_init_frame.ZeroCopyDecode(init_frame_msg)) return false;

  const auto& rsp_metadata = rsp_init_frame.stream_init_metadata.response_meta();
  if (rsp_metadata.ret() != TrpcRetCode::TRPC_INVOKE_SUCCESS) return false;

  return true;
}

trpc::Status TestTrpcClientStreamProvider::Read(NoncontiguousBuffer* msg, int timeout) {
  trpc::stream::ProtocolMessageMetadata meta{};
  NoncontiguousBuffer frame_msg;
  if (!Recv(meta, frame_msg)) return trpc::Status{-1, -1, "network recv error"};

  if (meta.stream_frame_type == static_cast<uint8_t>(trpc::stream::StreamMessageCategory::kStreamClose)) {
    return HandleClose(frame_msg);
  } else if (meta.stream_frame_type == static_cast<uint8_t>(trpc::stream::StreamMessageCategory::kStreamData)) {
    TrpcStreamDataFrameProtocol data_frame{};
    if (!data_frame.ZeroCopyDecode(frame_msg)) return trpc::Status{-1, -1, "decode error"};
    *msg = std::move(data_frame.body);
    return kSuccStatus;
  }
  return kUnknownErrorStatus;
}

trpc::Status TestTrpcClientStreamProvider::Write(NoncontiguousBuffer&& msg) {
  TrpcStreamDataFrameProtocol data_frame = CreateTrpcStreamDataFrameProtocol(options_.stream_id, msg);
  trpc::NoncontiguousBuffer send_buff;
  if (!data_frame.ZeroCopyEncode(send_buff)) return trpc::Status{-1, -1, "encode error"};
  if (options_.transport->Send(std::move(send_buff)) <= 0) return trpc::Status{-1, -1, "network send error"};
  return trpc::kSuccStatus;
}

trpc::Status TestTrpcClientStreamProvider::WriteDone() {
  TrpcStreamCloseMeta close_meta{};
  close_meta.set_close_type(TrpcStreamCloseType::TRPC_STREAM_CLOSE);
  TrpcStreamCloseFrameProtocol close_frame = CreateTrpcStreamCloseFrameProtocol(options_.stream_id, close_meta);
  trpc::NoncontiguousBuffer send_buff;
  if (!close_frame.ZeroCopyEncode(send_buff)) return trpc::Status{-1, -1, "encode error"};
  if (options_.transport->Send(std::move(send_buff)) <= 0) return trpc::Status{-1, -1, "network send error"};
  return trpc::kSuccStatus;
}

trpc::Status TestTrpcClientStreamProvider::HandleClose(NoncontiguousBuffer& msg) {
  TrpcStreamCloseFrameProtocol close_frame{};
  if (!close_frame.ZeroCopyDecode(msg)) return trpc::Status{-1, -1, "decode error"};

  const TrpcStreamCloseMeta& close_metadata = close_frame.stream_close_metadata;
  bool reset = (close_metadata.close_type() == TrpcStreamCloseType::TRPC_STREAM_RESET);
  Status status{close_metadata.ret(), close_metadata.func_ret(), std::string(close_metadata.msg())};
  if (reset) return status;
  if (status.GetFrameworkRetCode() != TrpcRetCode::TRPC_INVOKE_SUCCESS) return trpc::Status{-1, -1, "invoke failure"};
  // EOF
  return trpc::Status{-99, 0, "EOF"};
}

bool TestTrpcClientStreamProvider::Recv(trpc::stream::ProtocolMessageMetadata& meta, NoncontiguousBuffer& data) {
  if (recv_msgs_.empty()) {
    for (;;) {
      std::cerr << "recv stream frame" << std::endl;
      std::this_thread::sleep_for(20ms);

      if (options_.transport->Recv(recv_buff_) <= 0) return false;
      int check_ret = Check(recv_buff_, recv_msgs_);
      if (check_ret == -1)
        return false;
      else if (check_ret == 0)
        continue;
      if (recv_msgs_.size() >= 1) break;
    }
  }
  if (!Pick(recv_msgs_.front(), meta)) return false;

  bool ok = (meta.stream_id == options_.stream_id);
  ok &= meta.enable_stream;
  if (!ok) return false;

  data = std::any_cast<trpc::NoncontiguousBuffer&&>(std::move(recv_msgs_.front()));
  recv_msgs_.pop_front();
  return true;
}

int TestTrpcClientStreamProvider::Check(trpc::NoncontiguousBuffer& in, std::deque<std::any>& out) {
  return trpc::CheckTrpcProtocolMessage(conn_, in, out);
}

bool TestTrpcClientStreamProvider::Pick(const std::any& msg, trpc::stream::ProtocolMessageMetadata& metadata) {
  // Pick the metadata of protocol message (frame header).
  std::any data = trpc::TrpcProtocolMessageMetadata{};
  if (!trpc::PickTrpcProtocolMessageMetadata(msg, data)) {
    return false;
  }
  auto& meta = std::any_cast<trpc::TrpcProtocolMessageMetadata&>(data);
  metadata.data_frame_type = meta.data_frame_type;
  metadata.enable_stream = meta.enable_stream;
  metadata.stream_frame_type = meta.stream_frame_type;
  metadata.stream_id = meta.stream_id;
  return true;
}

}  // namespace trpc::testing
