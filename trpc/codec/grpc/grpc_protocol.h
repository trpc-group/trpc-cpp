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

#include <memory>
#include <string>
#include <utility>

#include "trpc/codec/grpc/http2/request.h"
#include "trpc/codec/grpc/http2/response.h"
#include "trpc/codec/protocol.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

// @brief Common HTTP2 header field names for the grpc protocol.
constexpr char kGrpcContentTypeName[] = "content-type";
constexpr char kGrpcEncodingName[] = "grpc-encoding";
constexpr char kGrpcAcceptEncodingName[] = "grpc-accept-encoding";
constexpr char kGrpcMessageTypeName[] = "grpc-message-type";
constexpr char kGrpcStatusName[] = "grpc-status";
constexpr char kGrpcMessageName[] = "grpc-message";
constexpr char kGrpcTimeoutName[] = "grpc-timeout";

// @brief Common Content-Type values for the grpc protocol.
constexpr char kGrpcContentTypeDefault[] = "application/grpc";
constexpr char kGrpcContentTypeProtobuf[] = "application/grpc+proto";
constexpr char kGrpcContentTypeJson[] = "application/grpc+json";

// Common Content-Encoding values for the grpc protocol.
constexpr char kGrpcContentEncodingGzip[] = "gzip";
constexpr char kGrpcContentEncodingSnappy[] = "snappy";
constexpr char kGrpcContentEncodingZlib[] = "zlib";

// @brief Indicates whether the grpc protocol supports compression.
constexpr uint8_t kGrpcNotSupportCompressed = 0;
constexpr uint8_t kGrpcSupportCompressed = 1;

/// @brief Status of grpc, reference to status code in gRPC.
/// @sa <https://github.com/grpc/grpc/blob/master/doc/statuscodes.md>
enum GrpcStatus {
  // OK 0, Not an error; returned on success.
  kGrpcOk = 0,

  // CANCELLED 1, The operation was cancelled, typically by the caller.
  kGrpcCancelled = 1,

  // UNKNOWN 2, Unknown error. For example, this error may be returned when a Status value
  // received from another address space belongs to an error space that is not known in
  // this address space. Also errors raised by APIs that do not return enough error information
  // may be converted to this error.
  kGrpcUnknown = 2,

  // INVALID_ARGUMENT 3, The client specified an invalid argument.
  // Note that this differs from FAILED_PRECONDITION.
  // INVALID_ARGUMENT indicates arguments that are problematic regardless of
  // the state of the system (e.g., a malformed file name).
  kGrpcInvalidArgument = 3,

  // DEADLINE_EXCEEDED 4, The deadline expired before the operation could complete.
  // For operations that change the state of the system, this error may be returned even
  // if the operation has completed successfully. For example, a successful response from
  // a server could have been delayed long
  kGrpcDeadlineExceeded = 4,

  // NOT_FOUND 5, Some requested entity (e.g., file or directory) was not found.
  // Note to server developers: if a request is denied for an entire class of users,
  // such as gradual feature rollout or undocumented allowlist, NOT_FOUND may be used.
  // If a request is denied for some users within a class of users,
  // such as user-based access control, PERMISSION_DENIED must be used.
  kGrpcNotFound = 5,

  // ALREADY_EXISTS 6, The entity that a client attempted to create (e.g., file or directory)
  // already exists.
  kGrpcAlreadyExists = 6,

  // PERMISSION_DENIED 7, The caller does not have permission to execute the specified operation.
  // PERMISSION_DENIED must not be used for rejections caused by exhausting some resource
  // (use RESOURCE_EXHAUSTED instead for those errors). PERMISSION_DENIED must not be used
  // if the caller can not be identified (use UNAUTHENTICATED instead for those errors).
  // This error code does not imply the request is valid or the requested entity exists or
  // satisfies other pre-conditions.
  kGrpcPermissionDenied = 7,

  // RESOURCE_EXHAUSTED 8, Some resource has been exhausted, perhaps a per-user quota,
  // or perhaps the entire file system is out of space.
  kGrpcResourceExhausted = 8,

  // FAILED_PRECONDITION 9, The operation was rejected because the system is not in a
  // state required for the operation's execution. For example, the directory to be deleted
  // is non-empty, an rmdir operation is applied to a non-directory, etc. Service implementors can
  // use the following guidelines to decide between FAILED_PRECONDITION, ABORTED, and UNAVAILABLE:
  // (a) Use UNAVAILABLE if the client can retry just the failing call.
  // (b) Use ABORTED if the client should retry at a higher level
  //   (e.g., when a client-specified test-and-set fails, indicating the client should restart a
  //   read-modify-write sequence).
  // (c) Use FAILED_PRECONDITION if the client should not retry until the system state has been
  //   explicitly fixed. E.g., if an "rmdir" fails because the directory is non-empty,
  //   FAILED_PRECONDITION should be returned since the client should not retry unless the files
  //   are deleted from the directory.
  kGrpcFailedPrecondition = 9,

  // ABORTED 10, The operation was aborted, typically due to a concurrency issue such as
  // a sequencer check failure or transaction abort. See the guidelines above for deciding
  // between FAILED_PRECONDITION, ABORTED, and UNAVAILABLE.
  kGrpcAborted = 10,

  // OUT_OF_RANGE 11, The operation was attempted past the valid range. E.g., seeking or reading
  // past end-of-file. Unlike INVALID_ARGUMENT, this error indicates a problem that may be fixed
  // if the system state changes. For example, a 32-bit file system will generate INVALID_ARGUMENT
  // if asked to read at an offset that is not in the range [0,2^32-1], but it will generate
  // OUT_OF_RANGE if asked to read from an offset past the current file size. There is a fair bit
  // of overlap between FAILED_PRECONDITION and OUT_OF_RANGE. We recommend using OUT_OF_RANGE
  // (the more specific error) when it applies so that callers who are iterating through a space
  // can easily look for an OUT_OF_RANGE error to detect when they are done.
  kGrpcOutOfRange = 11,

  // UNIMPLEMENTED 12, The operation is not implemented or is not supported/enabled in this service.
  kGrpcUnimplemented = 12,

  // INTERNAL 13, Internal errors. This means that some invariants expected by the underlying
  // system have been broken. This error code is reserved for serious errors.
  kGrpcInternal = 13,

  // UNAVAILABLE 14, The service is currently unavailable. This is most likely a transient
  // condition, which can be corrected by retrying with a backoff. Note that it is not always safe
  // to retry non-idempotent operations.
  kGrpcUnavailable = 14,

  // DATA_LOSS 15, Unrecoverable data loss or corruption.
  kGrpcDataLoss = 15,

  // UNAUTHENTICATED 16, The request does not have valid authentication credentials for the
  // operation.
  kGrpcUnauthenticated = 16,

  // MAX , not grpc status code.
  kGrpcMax,
};

/// @brief Returns the corresponding message for grpc status.
const char* GrpcStatusToString(int status);

/// @brif Conversion between grpc content-type and framework content-type.
uint32_t GrpcContentTypeToTrpcContentType(const std::string& content_type);
const char* TrpcContentTypeToGrpcContentType(uint32_t content_type);

/// @brief Conversion between grpc encoding-type and framework encoding-type.
uint32_t GrpcContentEncodingToTrpcContentEncoding(const std::string& content_encoding);
const char* TrpcContentEncodingToGrpcContentEncoding(uint32_t content_encoding);

/// @brief Converts grpc timeout values in text format to microseconds.
///
/// @param timeout_text Timeout text value, such as 1H (1 hour), 2M (2 minutes), 3S (3 seconds), 4m (4 milliseconds),
/// 5u (5 microseconds), 6n (6 nanoseconds).
int64_t GrpcTimeoutTextToUS(const std::string& timeout_text);

/// @brief Returns grpc user-agent.
const std::string& GetGrpcUserAgent();

/// @brief Conversion of uint32_t to big-endian format.
uint32_t Uint32BigEndianConvertor(uint32_t value);

// @brief gRPC message content with length prefix.
struct GrpcMessageContent {
  static constexpr uint8_t kPrefixByteSize = 5;
  static constexpr uint8_t kCompressedFieldByteSize = 1;
  static constexpr uint8_t kLengthFieldByteSize = 4;

  // Message compression flag. 0: uncompressed, 1: compressed using the algorithm specified by grpc-encoding.
  uint8_t compressed{0};
  // Content length.
  uint32_t length{0};
  // Content buffer.
  NoncontiguousBuffer content{};

  // @brief Decodes or encodes message content.
  bool Decode(NoncontiguousBuffer* buffer);
  bool Encode(NoncontiguousBuffer* buffer) const;

  // @brief Check for complete packets from the input binary sequence and copy them into out.
  static bool Check(NoncontiguousBuffer* in, NoncontiguousBuffer* out);
  // @brief Message byte length.
  size_t ByteSizeLong() const noexcept;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// @brief Request protocol message for Grpc Unary RPC.
class GrpcUnaryRequestProtocol : public Protocol {
 public:
  // @brief Request header.
  struct GrpcRequestHeader {
    // Request identifier. The grpc protocol does not have a "request id" field, but this is mainly used for
    // compatibility considerations.
    uint32_t request_id{0};
    // HTTP method (header name: ":method", value: "POST").
    std::string method{"POST"};
    // HTTP Scheme (header name: ":scheme", value: "http" or "https").
    std::string scheme{"http"};
    // HTTP authority (header name: ":authority", value: "$Host").
    std::string authority{"www.example.com"};
  };

 public:
  GrpcUnaryRequestProtocol() : http2_request_(http2::CreateRequest()) {}
  ~GrpcUnaryRequestProtocol() override = default;

  /// @brief Decodes or encodes a request message (zero-copy).
  /// @note No decoding or encoding action will be taken here; it will be handled by the HTTP2 protocol.
  bool ZeroCopyDecode(NoncontiguousBuffer& buffer) override { return false; }
  bool ZeroCopyEncode(NoncontiguousBuffer& buffer) override { return false; }

  /// @brief Gets or sets the unique ID of the request.
  /// @note The grpc protocol itself does not have a unique identifier for requests, but one has been added here for
  /// ease of handling.
  bool GetRequestId(uint32_t& req_id) const override {
    req_id = header_.request_id;
    return true;
  }
  bool SetRequestId(uint32_t req_id) override {
    header_.request_id = req_id;
    return true;
  }

  /// @brief Get size of message
  uint32_t GetMessageSize() const override { return content_.ByteSize(); }

  /// @brief Set/Get body payload of protocol message.
  NoncontiguousBuffer GetNonContiguousProtocolBody() override { return std::move(content_); }
  void SetNonContiguousProtocolBody(NoncontiguousBuffer&& buffer) override { content_ = std::move(buffer); }

  // @brief Gets the request header.
  // For internal use only.
  const GrpcRequestHeader& GetHeader() { return header_; }
  GrpcRequestHeader* GetMutableHeader() { return &header_; }

  // @brief Gets or sets HTTP2 request.
  // For internal use only.
  void SetHttp2Request(const http2::RequestPtr& request) { http2_request_ = request; }
  const http2::RequestPtr& GetHttp2Request() const { return http2_request_; }

 private:
  // Request header.
  GrpcRequestHeader header_{};
  // Request content.
  NoncontiguousBuffer content_{};
  // HTTP2 request
  http2::RequestPtr http2_request_{nullptr};
};
using GrpcUnaryRequestProtocolPtr = std::shared_ptr<GrpcUnaryRequestProtocol>;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// @brief Response protocol message for Grpc Unary RPC.
class GrpcUnaryResponseProtocol : public Protocol {
 public:
  struct GrpcResponseHeader {
    // Request identifier. The grpc protocol does not have a "request id" field, but this is mainly used for
    // compatibility considerations.
    uint32_t request_id{0};
    // Error return code for the request at the framework level.
    // The specific value corresponds to GrpcRetCode.
    int status{0};
    // Description of the call result used when the call fails.
    std::string error_msg{};
  };

 public:
  GrpcUnaryResponseProtocol() : http2_response_(http2::CreateResponse()) {}
  ~GrpcUnaryResponseProtocol() override = default;

  /// @brief Decodes or encodes a response message (zero-copy).
  /// @note No decoding or encoding action will be taken here; it will be handled by the HTTP2 protocol.
  bool ZeroCopyDecode(NoncontiguousBuffer& buffer) override { return false; }
  bool ZeroCopyEncode(NoncontiguousBuffer& buffer) override { return false; }

  /// @brief Gets or sets the unique ID of the request.
  /// @note The grpc protocol itself does not have a unique identifier for requests, but one has been added here for
  /// ease of handling.
  bool GetRequestId(uint32_t& req_id) const override {
    req_id = header_.request_id;
    return true;
  }
  bool SetRequestId(uint32_t req_id) override {
    header_.request_id = req_id;
    return true;
  }

  /// @brief Get size of message
  uint32_t GetMessageSize() const override { return content_.ByteSize(); }

  /// @brief Set/Get body payload of protocol message.
  NoncontiguousBuffer GetNonContiguousProtocolBody() override { return std::move(content_); }
  void SetNonContiguousProtocolBody(NoncontiguousBuffer&& buffer) override { content_ = std::move(buffer); }

  // @brief Gets the response header.
  // For internal use only.
  const GrpcResponseHeader& GetHeader() { return header_; }
  GrpcResponseHeader* GetMutableHeader() { return &header_; }

  // @brief Gets or sets the HTTP2 response.
  // For internal use only.
  void SetHttp2Response(const http2::ResponsePtr& response) { http2_response_ = response; }
  const http2::ResponsePtr& GetHttp2Response() const { return http2_response_; }

 private:
  // Response header.
  GrpcResponseHeader header_{};
  // Response content.
  NoncontiguousBuffer content_{};
  // HTTP2 response
  http2::ResponsePtr http2_response_{nullptr};
};
using GrpcUnaryResponseProtocolPtr = std::shared_ptr<GrpcUnaryResponseProtocol>;

/// @brief Metadata of grpc protocol message.
struct GrpcProtocolMessageMetadata {
  // Indicates it has streaming RPC.
  bool enable_stream{false};
  // Type of streaming-frame, e.g. INIT_FRAME, DATA_FRAME, CLOSE_FRAME, FEEDBACK_FRAME
  uint8_t stream_frame_type{0};
  // ID of stream.
  uint32_t stream_id{0};
};

namespace internal {
// For internal use only.
bool GetHeaderString(const http::HeaderPairs& header, std::string_view name, std::string* value);

// For internal use only.
bool GetHeaderInteger(const http::HeaderPairs& header, std::string_view name, int* value);
}  // namespace internal

}  // namespace trpc
