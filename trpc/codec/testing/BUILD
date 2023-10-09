licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "client_codec_testing",
    hdrs = ["client_codec_testing.h"],
    deps = [
        "//trpc/codec:client_codec",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "protocol_testing",
    hdrs = ["protocol_testing.h"],
    deps = [
        "//trpc/codec:protocol",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "server_codec_testing",
    hdrs = ["server_codec_testing.h"],
    deps = [
        ":protocol_testing",
        "//trpc/codec:server_codec",
        "@com_google_googletest//:gtest",
    ],
)
