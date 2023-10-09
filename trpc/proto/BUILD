licenses(["notice"])

package(
    default_visibility = [
        "//trpc:__subpackages__",
        "//test:__subpackages__",
    ]
)

proto_library(
    name = "trpc_proto",
    srcs = [
        "trpc.proto",
    ],
)

proto_library(
    name = "trpc_options_proto",
    srcs = [
        "trpc_options.proto",
    ],
    deps = [
        "@com_google_protobuf//:descriptor_proto",
    ],
)

cc_proto_library(
    name = "cc_trpc_options_proto",
    deps = [
        ":trpc_options_proto",
    ],
)
