licenses(["notice"])

package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

cc_proto_library(
    name = "test_serialization_cc_proto",
    deps = [":test_serialization_proto"],
)

proto_library(
    name = "test_serialization_proto",
    srcs = ["test_serialization.proto"],
)
