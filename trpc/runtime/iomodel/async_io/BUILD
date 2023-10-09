licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "async_io",
    srcs = select({
        "//trpc:trpc_include_async_io": ["async_io.cc"],
        "//conditions:default": [],
    }),
    hdrs = ["async_io.h"],
    defines = select({
        "//trpc:trpc_include_async_io": ["TRPC_BUILD_INCLUDE_ASYNC_IO"],
        "//conditions:default": [],
    }) + select({
        "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
        "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
        "//conditions:default": [],
    }),
    deps = [
        "//trpc/common/future",
        "//trpc/runtime/iomodel/reactor/common:eventfd_notifier",
        "//trpc/tvar",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/object_pool",
        "@liburing",
    ],
)

cc_test(
    name = "async_io_test",
    srcs = select({
        "//trpc:trpc_include_async_io": ["async_io_test.cc"],
        "//conditions:default": [],
    }),
    deps = [
        "//trpc/runtime/iomodel/reactor/common:epoll_poller",
        "//trpc/runtime/iomodel/reactor/default:reactor_impl",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ] + select({
        "//trpc:trpc_include_async_io": [":async_io"],
        "//conditions:default": [],
    }),
)
