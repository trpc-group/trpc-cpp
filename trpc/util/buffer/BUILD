licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "buffer",
    srcs = ["buffer.cc"],
    hdrs = ["buffer.h"],
    deps = [
        ":contiguous_buffer",
        ":noncontiguous_buffer",
    ],
)

cc_test(
    name = "buffer_test",
    srcs = ["buffer_test.cc"],
    deps = [
        ":buffer",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "contiguous_buffer",
    srcs = ["contiguous_buffer.cc"],
    hdrs = ["contiguous_buffer.h"],
    deps = [
        "//trpc/util:likely",
        "//trpc/util:ref_ptr",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "contiguous_buffer_test",
    srcs = ["contiguous_buffer_test.cc"],
    deps = [
        ":contiguous_buffer",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "noncontiguous_buffer",
    srcs = ["noncontiguous_buffer.cc"],
    hdrs = ["noncontiguous_buffer.h"],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_mem_pool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_mem_pool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":contiguous_buffer",
        "//trpc/util:align",
        "//trpc/util:check",
        "//trpc/util:likely",
        "//trpc/util:ref_ptr",
        "//trpc/util/buffer/memory_pool",
        "//trpc/util/internal:singly_linked_list",
        "//trpc/util/log:logging",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_test(
    name = "noncontiguous_buffer_test",
    srcs = ["noncontiguous_buffer_test.cc"],
    deps = [
        ":noncontiguous_buffer",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "zero_copy_stream",
    srcs = ["zero_copy_stream.cc"],
    hdrs = ["zero_copy_stream.h"],
    deps = [
        ":noncontiguous_buffer",
        "//trpc/util:check",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "zero_copy_stream_test",
    srcs = ["zero_copy_stream_test.cc"],
    deps = [
        ":contiguous_buffer",
        ":zero_copy_stream",
        "//trpc/util/buffer/testing:cc_message_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "noncontiguous_buffer_view",
    srcs = ["noncontiguous_buffer_view.cc"],
    hdrs = ["noncontiguous_buffer_view.h"],
    deps = [
        ":noncontiguous_buffer",
    ],
)

cc_test(
    name = "noncontiguous_buffer_view_test",
    srcs = ["noncontiguous_buffer_view_test.cc"],
    deps = [
        ":noncontiguous_buffer_view",
        "//trpc/util:random",
        "@com_google_googletest//:gtest_main",
    ],
)
