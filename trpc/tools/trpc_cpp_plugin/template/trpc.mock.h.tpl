// Code generated by protoc trpc plugin. DO NOT EDIT.
// source: {{ .ProtoFileName }}

{{ if .GenCCMock -}}
{{ $appName := .APPName -}}
{{ $serverName := .ServerName -}}

#pragma once

#include "gmock/gmock.h"

#include "{{ replaceSuffix .ProtoFileName ".proto" ".trpc.pb.h" }}"

{{- $namespaces := (split .PackageName ".") }}
{{ range $val := $namespaces }}
namespace {{$val}} {
{{- end }}

{{ range $service := .Services -}}
{{ $svcName := $service.Name -}}
{{- $hasStreamService:= or (hasClientStreamService $service.RPCs) (hasServerStreamService $service.RPCs) -}}

class Mock{{ $svcName }} : public {{ $svcName }} {
 public:
  {{- range $service.RPCs -}}
  {{- $rpcReqType := .RequestType -}}
	{{- $rpcRspType := .ResponseType -}}
  {{- $methodName := .Name -}}

  {{- if and .ClientStreaming (not .ServerStreaming) }}{{/*ClientStreaming*/}}
  MOCK_METHOD(::trpc::Status, {{ $methodName }}, (const ::trpc::ServerContextPtr&, const ::trpc::stream::StreamReader<{{ $rpcReqType }}>&, {{ $rpcRspType }}*));
  {{- else if and .ServerStreaming (not .ClientStreaming) }}{{/*ServerStreaming*/}}
  MOCK_METHOD(::trpc::Status, {{ $methodName }}, (const ::trpc::ServerContextPtr&, const {{ $rpcReqType }}&, ::trpc::stream::StreamWriter<{{ $rpcRspType }}>*));
  {{- else if and .ServerStreaming .ClientStreaming }}{{/*BidiStreaming*/}}
  MOCK_METHOD(::trpc::Status, {{ $methodName }}, (const ::trpc::ServerContextPtr&, const ::trpc::stream::StreamReader<{{ $rpcReqType }}>&, ::trpc::stream::StreamWriter<{{ $rpcRspType }}>*));
  {{- else }}{{/*Unary request*/}}
  MOCK_METHOD(::trpc::Status, {{ $methodName }}, (::trpc::ServerContextPtr, const {{ $rpcReqType }}*, {{ $rpcRspType }}*));
  {{- end -}}{{/* end of if streaming */}}

  {{- end -}}{{/* end of $service.RPCs range */}}
};

class MockAsync{{ $svcName }} : public Async{{ $svcName }} {
 public:
  {{- range $service.RPCs -}}
  {{- $rpcReqType := .RequestType -}}
	{{- $rpcRspType := .ResponseType -}}
  {{- $methodName := .Name -}}

  {{- if and .ClientStreaming (not .ServerStreaming) }}{{/*ClientStreaming*/}}
  MOCK_METHOD(::trpc::Status, {{ $methodName }}, (const ::trpc::ServerContextPtr&, const ::trpc::stream::StreamReader<{{ $rpcReqType }}>&, {{ $rpcRspType }}*));
  {{- else if and .ServerStreaming (not .ClientStreaming) }}{{/*ServerStreaming*/}}
  MOCK_METHOD(::trpc::Status, {{ $methodName }}, (const ::trpc::ServerContextPtr&, const {{ $rpcReqType }}&, ::trpc::stream::StreamWriter<{{ $rpcRspType }}>*));
  {{- else if and .ServerStreaming .ClientStreaming }}{{/*BidiStreaming*/}}
  MOCK_METHOD(::trpc::Status, {{ $methodName }}, (const ::trpc::ServerContextPtr&, const ::trpc::stream::StreamReader<{{ $rpcReqType }}>&, ::trpc::stream::StreamWriter<{{ $rpcRspType }}>*));
  {{- else }}{{/*Unary request*/}}
  MOCK_METHOD(::trpc::Status, {{ $methodName }}, (::trpc::ServerContextPtr, const {{ $rpcReqType }}*, {{ $rpcRspType }}*));
  {{- end -}}{{/* end of if streaming */}}

  {{- end -}}{{/* end of $service.RPCs range */}}
};

class Mock{{ $svcName }}ServiceProxy : public {{ $svcName }}ServiceProxy {
 public:
  {{- range $service.RPCs -}}
  {{- $rpcReqType := .RequestType -}}
	{{- $rpcRspType := .ResponseType -}}
  {{- $methodName := .Name }}

  {{- if and .ClientStreaming (not .ServerStreaming) }}{{/*ClientStreaming*/}}
  MOCK_METHOD(::trpc::stream::StreamWriter<{{ $rpcReqType }}>, {{ $methodName }}, (const ::trpc::ClientContextPtr&, {{ $rpcRspType }}*));
  {{- else if and .ServerStreaming (not .ClientStreaming) }}{{/*ServerStreaming*/}}
  MOCK_METHOD(::trpc::stream::StreamReader<{{ $rpcRspType }}>, {{ $methodName }}, (const ::trpc::ClientContextPtr&, const {{ $rpcReqType }}&));
  {{- else if and .ServerStreaming .ClientStreaming }}{{/*BidiStreaming*/}}
  MOCK_METHOD((::trpc::stream::StreamReaderWriter<{{ $rpcReqType }}, {{ $rpcRspType }}>), {{ $methodName }}, (const ::trpc::ClientContextPtr&));
  {{- else }}{{/*Unary request*/}}
  MOCK_METHOD(::trpc::Status, {{ $methodName }}, (const ::trpc::ClientContextPtr&, const {{ $rpcReqType }}&, {{ $rpcRspType }}*));
  MOCK_METHOD(::trpc::Future<{{ $rpcRspType }}>, Async{{ $methodName }}, (const ::trpc::ClientContextPtr&, const {{ $rpcReqType }}&));
  MOCK_METHOD(::trpc::Status, {{ $methodName }}, (const ::trpc::ClientContextPtr&, const {{ $rpcReqType }}&));
  {{- end -}}{{/* end of if streaming */}}

  {{- end -}}{{/* end of $service.RPCs range */}}
};

class MockAsync{{ $svcName }}ServiceProxy : public Async{{ $svcName }}ServiceProxy {
 public:
  {{- range $service.RPCs -}}
  {{- $rpcReqType := .RequestType -}}
	{{- $rpcRspType := .ResponseType -}}
  {{- $methodName := .Name }}

  {{- if and .ClientStreaming (not .ServerStreaming) }}{{/*ClientStreaming*/}}
  MOCK_METHOD(::trpc::Future<std::pair<::trpc::stream::AsyncWriterPtr<{{ $rpcReqType }}>, ::trpc::Future<{{ $rpcRspType }}>>>, {{ $methodName }}, (const ::trpc::ClientContextPtr&));
  {{- else if and .ServerStreaming (not .ClientStreaming) }}{{/*ServerStreaming*/}}
  MOCK_METHOD(::trpc::Future<::trpc::stream::AsyncReaderPtr<{{ $rpcRspType }}>>, {{ $methodName }}, (const ::trpc::ClientContextPtr&, {{ $rpcReqType }}&&));
  {{- else if and .ServerStreaming .ClientStreaming }}{{/*BidiStreaming*/}}
  MOCK_METHOD((::trpc::Future<::trpc::stream::AsyncReaderWriterPtr<{{ $rpcRspType }}, {{ $rpcReqType }}>>), {{ $methodName }}, (const ::trpc::ClientContextPtr&));
  {{- else }}{{/*Unary request*/}}
  MOCK_METHOD(::trpc::Future<{{ $rpcRspType }}>, {{ $methodName }}, (const ::trpc::ClientContextPtr&, const {{ $rpcReqType }}&));
  {{- end -}}{{/* end of if streaming */}}

  {{- end -}}{{/* end of $service.RPCs range */}}
};

{{- end -}}{{/* end of services range */}}

{{ range $val := (reverseStringSlice $namespaces)  }}
} // end namespace {{ $val }}
{{- end }}

{{- end }}{{/* end of if GenCCMock */}}
