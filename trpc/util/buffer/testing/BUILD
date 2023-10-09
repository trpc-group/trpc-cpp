licenses(["notice"])

package(default_visibility = ["//visibility:public"])

proto_library(
    name = "message_proto",
    srcs = [
        "message.proto",
    ],
)

cc_proto_library(
    name = "cc_message_proto",
    deps = [
        ":message_proto",
    ],
)
