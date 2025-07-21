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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "trpc/rpcz/span.h"

#include <sys/syscall.h>
#include <unistd.h>

#include <ostream>
#include <sstream>

#include "json/json.h"

#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace {

void UserSpanToJson(trpc::rpcz::Span* span_ptr, Json::Value& root) {
  // Assemble the ViewerEvent for the begin phase.
  {
    Json::Value begin_value;
    begin_value["name"] = Json::Value(span_ptr->ViewerName());
    begin_value["ph"] = Json::Value(span_ptr->CastPhaseTypeToStr(span_ptr->BeginViewerEvent().type));
    begin_value["ts"] = Json::Value(static_cast<Json::UInt64>(span_ptr->BeginViewerEvent().timestamp_us));
    begin_value["pid"] = Json::Value(static_cast<Json::UInt64>(0));
    begin_value["tid"] = Json::Value(static_cast<Json::UInt64>(span_ptr->BeginViewerEvent().thread_id));
    Json::Value args_value;
    for (const auto& iter : span_ptr->Attributes()) {
      args_value[iter.first] = Json::Value(iter.second);
    }
    begin_value["args"] = args_value;
    root.append(begin_value);
  }

  // Assemble the ViewerEvent for the end phase.
  {
    Json::Value end_value;
    end_value["name"] = Json::Value(span_ptr->ViewerName());
    end_value["ph"] = Json::Value(span_ptr->CastPhaseTypeToStr(span_ptr->EndViewerEvent().type));
    end_value["ts"] = Json::Value(static_cast<Json::UInt64>(span_ptr->EndViewerEvent().timestamp_us));
    end_value["pid"] = Json::Value(static_cast<Json::UInt64>(0));
    end_value["tid"] = Json::Value(static_cast<Json::UInt64>(span_ptr->EndViewerEvent().thread_id));
    Json::Value args_value;
    for (const auto& iter : span_ptr->Attributes()) {
      args_value[iter.first] = Json::Value(iter.second);
    }
    end_value["args"] = args_value;
    root.append(end_value);
  }

  for (auto* sub_span : span_ptr->SubSpans()) {
    UserSpanToJson(sub_span, root);
  }
}

}  // namespace

namespace trpc::rpcz {

Span::Span(const std::string& viewer_name) {
  SetViewerName(viewer_name);
  SetSpanType(SpanType::kSpanTypeUser);

  ViewerEvent begin_viewer_event;
  begin_viewer_event.type = PhaseType::kPhaseTypeBegin;
  begin_viewer_event.timestamp_us = trpc::GetSystemMicroSeconds();
  begin_viewer_event.thread_id = ::syscall(SYS_gettid);
  SetBeginViewerEvent(begin_viewer_event);
}

void Span::End() {
  ViewerEvent end_viewer_event;
  end_viewer_event.type = PhaseType::kPhaseTypeEnd;
  end_viewer_event.timestamp_us = trpc::GetSystemMicroSeconds();
  end_viewer_event.thread_id = ::syscall(SYS_gettid);
  SetEndViewerEvent(end_viewer_event);
}

Span* Span::CreateSubSpan(const std::string& viewer_name) {
  Span* span_ptr = trpc::object_pool::New<Span>(viewer_name);
  if (!span_ptr) {
    TRPC_FMT_ERROR("create span failed");
    return nullptr;
  }

  AppendSubSpan(span_ptr);
  return span_ptr;
}

bool Span::operator==(const Span& span) { return (this->span_id_ == span.SpanId()); }

std::string Span::CastCallTypeToStr(uint32_t call_type) const {
  switch (call_type) {
    case 0:
      return "UNARY_CALL";
    case 1:
      return "ONEWAY_CALL";
    case 2:
      return "CLIENT_STREAM_CALL";
    case 3:
      return "SERVER_STREAM_CALL";
    case 4:
      return "BIDI_STREAM_CALL";
    default:
      return "UNKNOW CALL_TYPE";
  }
  return "UNKNOW CALL_TYPE";
}

std::string Span::CastSpanTypeToStr(const SpanType& span_type) const {
  switch (span_type) {
    case SpanType::kSpanTypeServer:
      return "kSpanTypeServer";
    case SpanType::kSpanTypeClient:
      return "kSpanTypeClient";
    case SpanType::kSpanTypePrint:
      return "kSpanTypePrint";
    case SpanType::kSpanTypeUser:
      return "kSpanTypeUser";
    default:
      return "UNKNOW SPAN_TYPE";
  }
  return "UNKNOW SPAN_TYPE";
}

std::string Span::CastPhaseTypeToStr(const PhaseType& phase_type) const {
  switch (phase_type) {
    case PhaseType::kPhaseTypeBegin:
      return "B";
    case PhaseType::kPhaseTypeEnd:
      return "E";
    default:
      return "UNKNOW PHASE_TYPE";
  }
  return "UNKNOW PHASE_TYPE";
}

std::string Span::ClientSpanToString() {
  std::stringstream span_info;
  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(StartRpcInvokeRealUs()) << "   start send request("
            << RequestSize() << ") to " << RemoteName() << "(" << RemoteSide()
            << ") protocol=" << ProtocolName() << " span_id=" << SpanId() << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(StartTransInvokeRealUs()) << "   "
            << (StartTransInvokeRealUs() - StartRpcInvokeRealUs()) << "(us) start transport invoke"
            << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(StartSendRealUs()) << "   "
            << (StartSendRealUs() - StartTransInvokeRealUs()) << "(us) enter send queue" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(SendRealUs()) << "   "
            << (SendRealUs() - StartSendRealUs()) << "(us) leave send queue" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(SendDoneRealUs()) << "   "
            << (SendDoneRealUs() - SendRealUs()) << "(us) io send done" << std::endl;

  if (StartHandleRealUs() == 0) {
    span_info << "                             >>>>>>  invoke timeout, client timeout = " << Timeout()
              << "(ms)  <<<<<<" << std::endl;

    span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(TransInvokeDoneRealUs()) << "   "
              << (TransInvokeDoneRealUs() - SendDoneRealUs()) << "(us) finish transport invoke"
              << std::endl;

  } else {
    span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(StartHandleRealUs()) << "   "
              << (StartHandleRealUs() - SendDoneRealUs()) << "(us) enter recv queue" << std::endl;

    span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(HandleRealUs()) << "   "
              << (HandleRealUs() - StartHandleRealUs()) << "(us) leave recv func" << std::endl;

    span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(TransInvokeDoneRealUs()) << "   "
              << (TransInvokeDoneRealUs() - HandleRealUs()) << "(us) finish transport invoke" << std::endl;
  }

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(RpcInvokeDoneRealUs()) << "   "
            << (RpcInvokeDoneRealUs() - TransInvokeDoneRealUs()) << "(us) finish rpc invoke" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(RpcInvokeDoneRealUs())
            << "   0(us)Received response(" << ResponseSize() << ") from " << RemoteName() << "("
            << RemoteSide() << ")" << std::endl;
  return span_info.str();
}

void Span::FillRouteClientSpanInfo(const Span* sub_span, uint64_t last_log_us, std::ostream& span_info) {
  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(sub_span->StartRpcInvokeRealUs())
            << "   ------------------------start----------------------------->" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(sub_span->StartRpcInvokeRealUs()) << "   "
            << (sub_span->StartRpcInvokeRealUs() - last_log_us) << "(us) start send request("
            << sub_span->RequestSize() << ") to " << sub_span->RemoteName() << "(" << sub_span->RemoteSide()
            << ") protocol=" << sub_span->ProtocolName() << " span_id=" << sub_span->SpanId() << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(sub_span->StartTransInvokeRealUs()) << "   "
            << (sub_span->StartTransInvokeRealUs() - sub_span->StartRpcInvokeRealUs())
            << "(us) start transport invoke" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(sub_span->StartSendRealUs()) << "   "
            << (sub_span->StartSendRealUs() - sub_span->StartTransInvokeRealUs())
            << "(us) enter send queue" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(sub_span->SendRealUs()) << "   "
            << (sub_span->SendRealUs() - sub_span->StartSendRealUs()) << "(us) leave send queue" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(sub_span->SendDoneRealUs()) << "   "
            << (sub_span->SendDoneRealUs() - sub_span->SendRealUs()) << "(us) io send done" << std::endl;

  if (sub_span->StartHandleRealUs() == 0) {
    span_info << "                             >>>>>>  invoke timeout, client timeout = " << sub_span->Timeout()
              << "(ms)  <<<<<<" << std::endl;

    span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(sub_span->TransInvokeDoneRealUs()) << "   "
              << (sub_span->TransInvokeDoneRealUs() - sub_span->SendDoneRealUs())
              << "(us) finish transport invoke" << std::endl;

  } else {
    span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(sub_span->StartHandleRealUs()) << "   "
              << (sub_span->StartHandleRealUs() - sub_span->SendDoneRealUs())
              << "(us) enter recv queue" << std::endl;

    span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(sub_span->HandleRealUs()) << "   "
              << (sub_span->HandleRealUs() - sub_span->StartHandleRealUs())
              << "(us) leave recv func" << std::endl;

    span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(sub_span->TransInvokeDoneRealUs()) << "   "
              << (sub_span->TransInvokeDoneRealUs() - sub_span->HandleRealUs())
              << "(us) finish transport invoke" << std::endl;
  }

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(sub_span->RpcInvokeDoneRealUs()) << "   "
            << (sub_span->RpcInvokeDoneRealUs() - sub_span->TransInvokeDoneRealUs())
            << "(us) finish rpc invoke" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(sub_span->RpcInvokeDoneRealUs())
            << "   0(us)Received response(" << sub_span->ResponseSize() << ") from "
            << sub_span->RemoteName() << "(" << sub_span->RemoteSide() << ")" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(sub_span->RpcInvokeDoneRealUs())
            << "   <------------------------end-------------------------------" << std::endl;
}

std::string Span::ServerSpanToString() {
  std::stringstream span_info;
  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(ReceivedRealUs()) << "   Received request("
            << RequestSize() << ") from " << RemoteName() << "(" << RemoteSide()
            << ") protocol=" << ProtocolName() << " span_id=" << SpanId() << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(StartHandleRealUs()) << "   "
            << (StartHandleRealUs() - ReceivedRealUs()) << "(us) enter recv queue" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(HandleRealUs()) << "   "
            << (HandleRealUs() - StartHandleRealUs()) << "(us) leave recv queue" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(StartCallbackRealUs()) << "   "
            << (StartCallbackRealUs() - HandleRealUs()) << "(us) enter customer func" << std::endl;

  // Print custom func logic for rpcz.
  uint64_t last_log_real_us = StartCallbackRealUs();
  for (auto sub_span : SubSpans()) {
    if (sub_span->Type() == SpanType::kSpanTypePrint) {
      span_info << sub_span->CustomLogs();
    } else if (sub_span->Type() == SpanType::kSpanTypeClient) {
      FillRouteClientSpanInfo(sub_span, last_log_real_us, span_info);
    }
    last_log_real_us = sub_span->LastLogRealUs();
  }

  uint64_t leave_func_cost = 0;
  if (SubSpans().size() == 0) {
    leave_func_cost = CallbackRealUs() - StartCallbackRealUs();
  } else {
    leave_func_cost = CallbackRealUs() - SubSpans().back()->LastLogRealUs();
  }

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(CallbackRealUs()) << "   " << leave_func_cost
            << "(us) leave customer func" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(StartEncodeRealUs()) << "   "
            << (StartEncodeRealUs() - CallbackRealUs()) << "(us) start encode" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(StartSendRealUs()) << "   "
            << (StartSendRealUs() - StartEncodeRealUs()) << "(us) enter send queue" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(SendRealUs()) << "   "
            << (SendRealUs() - StartSendRealUs()) << "(us) leave send queue" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(SendDoneRealUs()) << "   "
            << (SendDoneRealUs() - SendRealUs()) << "(us) io send done" << std::endl;

  span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(SendDoneRealUs()) << "   Send response("
            << ResponseSize() << ") to " << RemoteName() << "(" << RemoteSide() << ")" << std::endl;

  return span_info.str();
}

std::string Span::UserSpanToString() {
  Json::Value root_value;
  UserSpanToJson(this, root_value);
  Json::StreamWriterBuilder json_builder;
  json_builder["indentation"] = "  ";
  json_builder["commentStyle"] = "None";

  return Json::writeString(json_builder, root_value);
}

void Span::Display() {
  std::stringstream os;
  os << std::endl;
  os << "------------------------------------------" << std::endl;
  os << "print Span info:" << std::endl;
  os << "trace_id:" << TraceId() << std::endl;
  os << "span_id:" << SpanId() << std::endl;
  os << "ReceivedRealUs:" << trpc::TimeStringHelper::ConvertMicroSecsToStr(ReceivedRealUs())
     << ", int:" << ReceivedRealUs() << std::endl;
  os << "call_type:" << CastCallTypeToStr(CallType()) << std::endl;
  os << "remote_side:" << RemoteSide() << std::endl;
  os << "protocol_name:" << ProtocolName() << std::endl;
  os << "request_size:" << RequestSize() << std::endl;
  os << "type:" << CastSpanTypeToStr(Type()) << std::endl;
  os << "full_method_name:" << FullMethodName() << std::endl;
  os << "remote_name:" << RemoteName() << std::endl;
  os << "start_handle_real_us_:" << trpc::TimeStringHelper::ConvertMicroSecsToStr(StartHandleRealUs())
     << ", int:" << StartHandleRealUs() << std::endl;
  os << "handle_real_us_:" << trpc::TimeStringHelper::ConvertMicroSecsToStr(HandleRealUs())
     << ", int:" << HandleRealUs() << std::endl;
  os << "start_callback_real_us:" << trpc::TimeStringHelper::ConvertMicroSecsToStr(StartCallbackRealUs())
     << ", int:" << StartCallbackRealUs() << std::endl;
  os << "start_encode_real_us_:" << trpc::TimeStringHelper::ConvertMicroSecsToStr(StartEncodeRealUs())
     << ", int:" << StartEncodeRealUs() << std::endl;
  os << "start_send_real_us:" << trpc::TimeStringHelper::ConvertMicroSecsToStr(StartSendRealUs())
     << ", int:" << StartSendRealUs() << std::endl;
  os << "send_real_us:" << trpc::TimeStringHelper::ConvertMicroSecsToStr(SendRealUs()) << ", int:" << SendRealUs()
     << std::endl;
  os << "error_code:" << ErrorCode() << std::endl;
  os << "response_size:" << ResponseSize() << std::endl;
  os << "viewer_name:" << ViewerName() << std::endl;
  os << "------------------------------------------" << std::endl;
  TRPC_FMT_DEBUG("Span info:{}", os.str());
}

}  // namespace trpc::rpcz
#endif
