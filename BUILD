cc_library(
    name = "abstract_generator",
    srcs = ["abstract_generator.cc"],
    hdrs = [
      "abstract_generator.h",
      "config.h"
    ],
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


py_binary(
    name = "generate",
    srcs = ["generate.py"],
    data = [
      ":cpp_generator",
      ":go_generator",
      ],
)
