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

#include "trpc/stream/http/common/stream.h"

#include "trpc/stream/http/common.h"
#include "trpc/util/http/common.h"
#include "trpc/util/string_helper.h"

namespace trpc::stream {

HttpCommonStream::~HttpCommonStream() {
  if (!IsStateTerminate()) {
    TRPC_FMT_ERROR("Unexpected read_state: {}, write_state: {}", StreamStateToString(read_state_),
                   StreamStateToString(write_state_));
  }
}

bool HttpCommonStream::IsStateTerminate() { return read_state_ == State::kClosed && write_state_ == State::kClosed; }

void HttpCommonStream::OnConnectionClosed() {
  if (status_.OK() && !IsStateTerminate()) {
    Status status(GetNetWorkErrorCode(), 0, "network error");
    TRPC_FMT_ERROR("server: {}, conn_id: {}, {}", options_.server_mode, options_.connection_id, status.ErrorMessage());
    // push the error of connection disconnected
    OnError(status);
    // stop and remove the stream
    Stop();
  }
}

std::string_view HttpCommonStream::StreamStateToString(State state) {
  switch (state) {
    case State::kClosed:
      return "closed";
    case State::kHalfClosed:
      return "half-closed";
    case State::kIdle:
      return "idle";
    case State::kInit:
      return "init";
    case State::kOpen:
      return "open";
    default:
      TRPC_ASSERT(false && "No such State");
  }
}

std::string_view HttpCommonStream::StreamActionToString(Action action) {
  switch (action) {
    case Action::kSendStartLine:
      return "send start line";
    case Action::kSendHeader:
      return "send header";
    case Action::kSendData:
      return "send data";
    case Action::kSendEof:
      return "send eof";
    case Action::kSendTrailer:
      return "send trailer";
    case Action::kHandleStartLine:
      return "handle start line";
    case Action::kHandleHeader:
      return "handle header";
    case Action::kHandleData:
      return "handle data";
    case Action::kHandleEof:
      return "handle eof";
    case Action::kHandleTrailer:
      return "handle trailer";
    default:
      TRPC_ASSERT(false && "No such Action");
  }
}

bool HttpCommonStream::CheckState(State state, Action action) {
  bool ok{true};
  std::string expected_state;

  switch (action) {
    case Action::kHandleStartLine:
    case Action::kSendStartLine:
      ok = (state == State::kIdle);
      if (!ok) expected_state.append(StreamStateToString(State::kIdle));
      break;
    case Action::kHandleHeader:
    case Action::kSendHeader:
      ok = (state == State::kInit);
      if (!ok) expected_state.append(StreamStateToString(State::kInit));
      break;
    case Action::kHandleData:
    case Action::kSendData:
    case Action::kHandleEof:
      ok = (state == State::kOpen);
      if (!ok) expected_state.append(StreamStateToString(State::kOpen));
      break;
    case Action::kSendEof: {
      State check_state = State::kOpen;
      if (write_mode_ == DataMode::kContentLength) {
        // in length mode, after sending the data, the state will change to close
        check_state = State::kClosed;
      }
      ok = (state == check_state);
      if (!ok) expected_state.append(StreamStateToString(check_state));
      break;
    }
    case Action::kHandleTrailer:
    case Action::kSendTrailer:
      ok = state == State::kHalfClosed;
      if (!ok) expected_state.append(StreamStateToString(State::kHalfClosed));
      break;
    default:
      ok = false;
      expected_state.append("error state");
  }

  if (TRPC_UNLIKELY(!ok)) {
    std::string error_msg{"bad stream state"};
    error_msg.append(", action: ").append(StreamActionToString(action));
    error_msg.append(", stream state: ").append(StreamStateToString(state));
    error_msg.append(", expected state: ").append(expected_state);
    TRPC_FMT_ERROR(error_msg);

    Status status{TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR, 0, std::move(error_msg)};
    OnError(status);
  }

  return ok;
}

void HttpCommonStream::Stop() {
  if (!is_stop_) {
    read_state_ = write_state_ = State::kClosed;
    is_stop_ = true;
    options_.stream_handler->RemoveStream(0);
  }
}

RetCode HttpCommonStream::HandleHeader(http::HttpHeader* header, HttpStreamHeader::MetaData* meta) {
  TRPC_FMT_DEBUG("server: {}, conn_id: {}, handle header begin", options_.server_mode, options_.connection_id);

  if (TRPC_UNLIKELY(!CheckState(read_state_, Action::kHandleHeader))) {
    return RetCode::kError;
  }

  // setup the read mode
  if (meta->is_chunk || (!meta->is_chunk && meta->content_length > 0)) {
    read_mode_ = meta->is_chunk ? DataMode::kChunked : DataMode::kContentLength;
    read_state_ = State::kOpen;
  } else {
    read_mode_ = DataMode::kNoData;
    read_state_ = State::kClosed;
  }
  // setup if has trailer
  read_has_trailer_ = meta->has_trailer;

  OnHeader(header);

  TRPC_FMT_DEBUG("server: {}, conn_id: {}, handle header end", options_.server_mode, options_.connection_id);

  return RetCode::kSuccess;
}

RetCode HttpCommonStream::HandleData(NoncontiguousBuffer* data) {
  TRPC_FMT_DEBUG("server: {}, conn_id: {}, handle data begin", options_.server_mode, options_.connection_id);

  if (TRPC_UNLIKELY(!CheckState(read_state_, Action::kHandleData))) {
    return RetCode::kError;
  }

  // Process the data when there has data and it is readable, otherwise discard the data.
  if (TRPC_LIKELY(read_mode_ != DataMode::kNoData)) {
    OnData(data);
  } else {
    TRPC_LOG_WARN("Unexpected read http body, header should contain `Content-Length` or `Transfer-Encoding: chunked`");
  }

  TRPC_FMT_DEBUG("server: {}, conn_id: {}, handle data end", options_.server_mode, options_.connection_id);

  return RetCode::kSuccess;
}

RetCode HttpCommonStream::HandleEof() {
  TRPC_FMT_DEBUG("server: {}, conn_id: {}, handle eof begin", options_.server_mode, options_.connection_id);

  if (TRPC_UNLIKELY(!CheckState(read_state_, Action::kHandleEof))) {
    return RetCode::kError;
  }

  OnEof();

  // if there has trailer, the stream will not be completely closed.
  if (read_mode_ == DataMode::kChunked && read_has_trailer_) {
    read_state_ = State::kHalfClosed;
  } else {
    read_state_ = State::kClosed;
  }

  TRPC_FMT_DEBUG("server: {}, conn_id: {}, handle eof end", options_.server_mode, options_.connection_id);

  return RetCode::kSuccess;
}

RetCode HttpCommonStream::HandleTrailer(http::HttpHeader* trailer) {
  TRPC_FMT_DEBUG("server: {}, conn_id: {}, handle trailer begin", options_.server_mode, options_.connection_id);

  if (TRPC_UNLIKELY(!CheckState(read_state_, Action::kHandleTrailer))) {
    return RetCode::kError;
  }

  OnTrailer(trailer);

  read_state_ = State::kClosed;

  TRPC_FMT_DEBUG("server: {}, conn_id: {}, handle trailer end", options_.server_mode, options_.connection_id);

  return RetCode::kSuccess;
}

RetCode HttpCommonStream::HandleRecvMessage(HttpStreamFramePtr&& msg) {
  switch (msg->GetFrameType()) {
    case HttpStreamFrame::HttpStreamFrameType::kHeader: {
      auto frame = static_cast<HttpStreamHeader*>(msg.Get());
      return HandleHeader(frame->GetMutableHeader(), frame->GetMutableMetaData());
    }
    case HttpStreamFrame::HttpStreamFrameType::kData: {
      auto frame = static_cast<HttpStreamData*>(msg.Get());
      return HandleData(frame->GetMutableData());
    }
    case HttpStreamFrame::HttpStreamFrameType::kEof: {
      return HandleEof();
    }
    case HttpStreamFrame::HttpStreamFrameType::kTrailer: {
      auto frame = static_cast<HttpStreamTrailer*>(msg.Get());
      return HandleTrailer(frame->GetMutableHeader());
    }
    case HttpStreamFrame::HttpStreamFrameType::kFullMessage: {
      auto full_msg = static_cast<HttpStreamFullMessage*>(msg.Get());
      has_full_message_ = true;
      for (std::any& it : *full_msg->GetMutableFrames()) {
        RetCode ret = HandleRecvMessage(std::any_cast<HttpStreamFramePtr>(std::move(it)));
        if (ret == RetCode::kError) {
          return ret;
        }
      }
      return RetCode::kSuccess;
    }
    default: {
      TRPC_ASSERT(false && "UnExpected frame type");
    }
  }
}

RetCode HttpCommonStream::HandleSendMessage(HttpStreamFramePtr&& msg, NoncontiguousBuffer* out) {
  switch (msg->GetFrameType()) {
    case HttpStreamFrame::HttpStreamFrameType::kHeader: {
      auto frame = static_cast<HttpStreamHeader*>(msg.Get());
      return PreSendHeader(frame->GetMutableHeader(), out);
    }
    case HttpStreamFrame::HttpStreamFrameType::kData: {
      auto frame = static_cast<HttpStreamData*>(msg.Get());
      return PreSendData(frame->GetMutableData(), out);
    }
    case HttpStreamFrame::HttpStreamFrameType::kEof: {
      return PreSendEof(out);
    }
    case HttpStreamFrame::HttpStreamFrameType::kTrailer: {
      auto frame = static_cast<HttpStreamTrailer*>(msg.Get());
      return PreSendTrailer(frame->GetMutableHeader(), out);
    }
    default: {
      TRPC_ASSERT(false && "UnExpected frame type");
    }
  }
}

RetCode HttpCommonStream::PreSendHeader(http::HttpHeader* header, NoncontiguousBuffer* out) {
  TRPC_FMT_DEBUG("server: {}, conn_id: {}, send header begin", options_.server_mode, options_.connection_id);

  if (TRPC_UNLIKELY(!CheckState(write_state_, Action::kSendHeader))) {
    return RetCode::kError;
  }

  // setup the write mode
  if (header->Get(http::kHeaderTransferEncoding).find(http::kTransferEncodingChunked) != std::string::npos) {
    write_mode_ = DataMode::kChunked;
    // It is only necessary to use trailers in chunked mode.
    std::vector<std::string_view> trailers = header->Values(http::kTrailer);
    if (!trailers.empty()) {
      for (const auto& trailer : trailers) {
        write_trailers_.insert(std::string(trailer));
      }
    }
    write_state_ = State::kOpen;
  } else if (const std::string& val = header->Get(http::kHeaderContentLength); !val.empty()) {
    std::optional<std::size_t> content_length = trpc::TryParse<std::size_t>(val);
    if (TRPC_UNLIKELY(!content_length.has_value())) {
      std::string err_msg = "Error parse `Content-Length: " + val + "`";
      TRPC_FMT_ERROR(err_msg);
      Status status{TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR, 0, std::move(err_msg)};
      OnError(status);
      return RetCode::kError;
    }
    write_content_length_ = content_length.value();
    if (write_content_length_ > 0) {
      write_mode_ = DataMode::kContentLength;
      write_state_ = State::kOpen;
    } else {
      write_mode_ = DataMode::kNoData;
      write_state_ = State::kClosed;
    }
  } else {
    write_mode_ = DataMode::kNoData;
    write_state_ = State::kClosed;
  }

  // encode the header
  NoncontiguousBufferBuilder builder;
  if (header->FlatPairsCount() > 0) {
    builder.Append(header->ToString());
  } else {
    builder.Append(http::kEmptyLine);
  }
  builder.Append(http::kEmptyLine);
  *out = builder.DestructiveGet();

  TRPC_FMT_DEBUG("server: {}, conn_id: {}, send header end", options_.server_mode, options_.connection_id);

  return RetCode::kSuccess;
}

RetCode HttpCommonStream::PreSendData(NoncontiguousBuffer* data, NoncontiguousBuffer* out) {
  TRPC_FMT_DEBUG("server: {}, conn_id: {}, send data begin", options_.server_mode, options_.connection_id);

  if (TRPC_UNLIKELY(!CheckState(write_state_, Action::kSendData))) {
    return RetCode::kError;
  }

  if (write_mode_ == DataMode::kChunked) {
    NoncontiguousBufferBuilder builder;
    builder.Append(HttpChunkHeader(data->ByteSize()));
    builder.Append(*data);
    builder.Append(http::kEndOfChunkMarker);
    *out = builder.DestructiveGet();
  } else if (write_mode_ == DataMode::kContentLength) {
    write_bytes_ += data->ByteSize();
    // check if the amount of data already sent is greater than the Content-Length.
    if (TRPC_UNLIKELY(write_bytes_ > write_content_length_)) {
      std::string err_msg = "Current Write " + std::to_string(write_bytes_) +
                            " bytes bigger than `Content-Length: " + std::to_string(write_content_length_) + "`";
      TRPC_FMT_ERROR(err_msg);
      Status status{TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR, 0, std::move(err_msg)};
      OnError(status);
      return RetCode::kError;
    } else if (write_bytes_ == write_content_length_) {
      write_state_ = State::kClosed;
    }
    *out = std::move(*data);
  }

  TRPC_FMT_DEBUG("server: {}, conn_id: {}, send data end", options_.server_mode, options_.connection_id);

  return RetCode::kSuccess;
}

RetCode HttpCommonStream::PreSendEof(NoncontiguousBuffer* out) {
  TRPC_FMT_DEBUG("server: {}, conn_id: {}, send eof begin", options_.server_mode, options_.connection_id);

  if (TRPC_UNLIKELY(!CheckState(write_state_, Action::kSendEof))) {
    return RetCode::kError;
  }

  if (write_mode_ == DataMode::kChunked) {
    NoncontiguousBufferBuilder builder;
    builder.Append(http::kEndOfChunkTransferingMarker);
    if (write_trailers_.size() == 0) {
      // if there is no trailer, send the chunk termination character and empty trailer
      builder.Append(http::kEmptyLine);
      write_state_ = State::kClosed;
    } else {
      // if there has trailer, only send the chunk termination character
      write_state_ = State::kHalfClosed;
    }
    *out = builder.DestructiveGet();
  } else if (write_mode_ == DataMode::kContentLength) {
    // check if the amount of data already sent is equal to the Content-Length.
    if (TRPC_UNLIKELY(write_bytes_ != write_content_length_)) {
      std::string err_msg = "Write " + std::to_string(write_bytes_) +
                            " bytes not equal to `Content-Length: " + std::to_string(write_content_length_) + "`";
      TRPC_FMT_ERROR(err_msg);
      Status status{TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR, 0, std::move(err_msg)};
      OnError(status);
      return RetCode::kError;
    }
  }

  TRPC_FMT_DEBUG("server: {}, conn_id: {}, send eof end", options_.server_mode, options_.connection_id);

  return RetCode::kSuccess;
}

RetCode HttpCommonStream::PreSendTrailer(http::HttpHeader* header, NoncontiguousBuffer* out) {
  TRPC_FMT_DEBUG("server: {}, conn_id: {}, send trailer begin", options_.server_mode, options_.connection_id);

  if (TRPC_UNLIKELY(!CheckState(write_state_, Action::kSendTrailer))) {
    return RetCode::kError;
  }
  // check if the trailers valid
  if (header->FlatPairsCount() != write_trailers_.size()) {
    std::string err_msg = "Expected " + std::to_string(write_trailers_.size()) + " trailers, but got " +
                          std::to_string(header->FlatPairsCount());
    TRPC_FMT_ERROR(err_msg);
    Status status{TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR, 0, std::move(err_msg)};
    OnError(status);
    return RetCode::kError;
  }
  auto pairs = header->Pairs();
  for (const auto& it : pairs) {
    if (write_trailers_.find(std::string(it.first)) == write_trailers_.end()) {
      std::string err_msg = "UnExpected trailer `" + std::string(it.first) + "` found";
      TRPC_FMT_ERROR(err_msg);
      Status status{TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR, 0, std::move(err_msg)};
      OnError(status);
      return RetCode::kError;
    }
  }
  // encode the trailer
  NoncontiguousBufferBuilder builder;
  if (header->FlatPairsCount() > 0) {
    builder.Append(header->ToString());
  } else {
    builder.Append(http::kEmptyLine);
  }
  builder.Append(http::kEmptyLine);
  *out = builder.DestructiveGet();

  write_state_ = State::kClosed;

  TRPC_FMT_DEBUG("server: {}, conn_id: {}, send trailer end", options_.server_mode, options_.connection_id);

  return RetCode::kSuccess;
}

}  // namespace trpc::stream
