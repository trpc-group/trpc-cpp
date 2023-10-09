licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hazptr",
    srcs = [
        "entry_cache.cc",
        "hazptr_domain.cc",
        "hazptr_object.cc",
    ],
    hdrs = [
        "entry.h",
        "entry_cache.h",
        "hazptr.h",
        "hazptr_domain.h",
        "hazptr_object.h",
    ],
    deps = [
        "//trpc/util/internal:never_destroyed",
        "//trpc/util/internal:time_keeper",
        "//trpc/util/thread/internal:memory_barrier",
    ],
)
