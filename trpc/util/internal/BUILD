licenses(["notice"])

package(default_visibility = ["//trpc:__subpackages__"])

cc_library(
    name = "casting",
    hdrs = ["casting.h"],
    deps = [
        ":demangle",
        ":index_alloc",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "casting_test",
    srcs = ["casting_test.cc"],
    deps = [
        ":casting",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "demangle",
    srcs = ["demangle.cc"],
    hdrs = ["demangle.h"],
    deps = [
        "//trpc/util:deferred",
    ],
)

cc_test(
    name = "demangle_test",
    srcs = ["demangle_test.cc"],
    deps = [
        ":demangle",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "doubly_linked_list",
    hdrs = ["doubly_linked_list.h"],
    deps = [
        "//trpc/util:check",
        "//trpc/util:likely",
    ],
)

cc_test(
    name = "doubly_linked_list_test",
    srcs = ["doubly_linked_list_test.cc"],
    deps = [
        ":doubly_linked_list",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "index_alloc",
    srcs = ["index_alloc.cc"],
    hdrs = ["index_alloc.h"],
    deps = [
        "//trpc/util/internal:never_destroyed",
    ],
)

cc_library(
    name = "index_alloc_test",
    srcs = ["index_alloc_test.cc"],
    deps = [
        ":index_alloc",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "meta",
    hdrs = ["meta.h"],
)

cc_library(
    name = "never_destroyed",
    hdrs = ["never_destroyed.h"],
    deps = [
    ],
)

cc_test(
    name = "never_destroyed_test",
    srcs = ["never_destroyed_test.cc"],
    deps = [
        ":never_destroyed",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "singly_linked_list",
    hdrs = ["singly_linked_list.h"],
    deps = [
        "//trpc/util:check",
        "//trpc/util:likely",
    ],
)

cc_test(
    name = "singly_linked_list_test",
    srcs = ["singly_linked_list_test.cc"],
    deps = [
        ":singly_linked_list",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "time_keeper",
    srcs = ["time_keeper.cc"],
    hdrs = ["time_keeper.h"],
    deps = [
        "//trpc/util:function",
        "//trpc/util:likely",
        "//trpc/util:ref_ptr",
        "//trpc/util/internal:never_destroyed",
    ],
)

cc_test(
    name = "time_keeper_test",
    srcs = ["time_keeper_test.cc"],
    deps = [
        ":time_keeper",
        "//trpc/util/chrono",
        "@com_google_googletest//:gtest_main",
    ],
)
