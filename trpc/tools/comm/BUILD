licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "utils",
    srcs = ["utils.cc"],
    hdrs = ["utils.h"],
    deps = [
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protoc_lib",
    ],
)

proto_library(
    name = "trpc_cpp_options_proto",
    srcs = ["trpc_cpp_options.proto"],
    deps = ["@com_google_protobuf//:descriptor_proto"],
)
