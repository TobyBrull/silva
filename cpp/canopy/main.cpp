#include "main.hpp"

#include "env_context.hpp"

namespace silva {
  expected_t<void>
  main_inner(const int argc, char* argv[], expected_t<void> (*real_main)(span_t<string_view_t>))
  {
    silva::env_context_t env_context_environ;
    SILVA_EXPECT_FWD(silva::env_context_fill_environ(&env_context_environ));
    silva::env_context_t env_context_cmdline;
    SILVA_EXPECT_FWD(silva::env_context_fill_cmdline(&env_context_cmdline, argc, argv));

    array_t<string_view_t> cmdline_args;
    for (int i = 0; i < argc; ++i) {
      const string_view_t arg_str{argv[i]};
      if (!arg_str.starts_with("--")) {
        cmdline_args.push_back(arg_str);
      }
    }
    return (*real_main)(cmdline_args);
  }

  int main(const int argc, char* argv[], expected_t<void> (*real_main)(span_t<string_view_t>))
  {
    const silva::expected_t<void> result = main_inner(argc, argv, real_main);
    if (!result) {
      const silva::error_t& error = result.error();
      byte_sink_cfile_t _stderr(stderr);
      _stderr.format("ERROR ({}):\n", pretty_string(error.level));
      silva::pretty_write(error, &_stderr);
      _stderr.write_str("\n");
      return static_cast<int>(error.level);
    }
    else {
      return 0;
    }
  }
}
