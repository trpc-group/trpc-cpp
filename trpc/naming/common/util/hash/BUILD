# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hash_func",
    srcs = ["hash_func.cc"],
    hdrs = ["hash_func.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/naming/common/util/hash:city",
        "//trpc/naming/common/util/hash:md5",
        "//trpc/naming/common/util/hash:murmurhash3",
    ],
)

cc_library(
    name = "murmurhash3",
    srcs = ["murmurhash3.cc"],
    hdrs = ["murmurhash3.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [],
)

cc_library(
    name = "city",
    srcs = ["city.cc"],
    hdrs = ["city.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [],
)

cc_library(
    name = "md5",
    srcs = ["md5.cc"],
    hdrs = ["md5.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [],
)
