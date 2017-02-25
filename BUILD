load("@io_bazel_rules_go//go:def.bzl", "go_prefix")
go_prefix("github.com/ncteisen/grpc-prober-generators")

cc_library(
    name = "abstract_generator",
    srcs = ["abstract_generator.cc"],
    hdrs = [
      "abstract_generator.h",
      "config.h"
    ],
    linkopts = [
      "-lprotobuf",
      "-lprotoc"
    ]
)

cc_binary(
    name = "cpp_generator",
    srcs = ["cpp_generator_plugin.cc"],
    deps = [":abstract_generator"],
)

cc_binary(
    name = "go_generator",
    srcs = ["go_generator_plugin.cc"],
    deps = [":abstract_generator"],
)


cc_binary(
    name = "python_generator",
    srcs = ["python_generator_plugin.cc"],
    deps = [":abstract_generator"],
)
