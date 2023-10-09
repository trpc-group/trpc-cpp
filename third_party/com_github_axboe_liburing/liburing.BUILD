licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "liburing",
    srcs = [
        "liburing.a",
    ],
    hdrs = glob([
        "src/include/**/*.h",
    ]),
    includes = [
        "src/include/",
    ],
    deps = [
        "liburing-compat",
    ],
)

cc_library(
    name = "liburing-compat",
    hdrs = [
        "compat.h",
    ],
    include_prefix = "liburing",
)

genrule(
    name = "liburing-build",
    srcs = glob(
        ["**/*"],
        exclude = ["bazel-*"],
    ),
    outs = [
        "liburing.a",
        "compat.h",
    ],
    cmd = """
        SRCROOT=$$(dirname $(location configure))
        echo $$SRCROOT
        pushd $$SRCROOT > /dev/null
            ./configure && make -C src ENABLE_SHARED=0
        popd > /dev/null
        [ -f $$SRCROOT/src/liburing.a ] && cp $$SRCROOT/src/liburing.a $(location liburing.a)
        [ -f $$SRCROOT/src/include/liburing/compat.h ] && cp $$SRCROOT/src/include/liburing/compat.h $(location compat.h)
    """,
    local = True,
)
