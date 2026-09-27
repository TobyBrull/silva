#include "main.hpp"

#include "env_context.hpp"

namespace silva {
  int main(const int argc, char* argv[], expected_t<void> (*real_main)(span_t<string_view_t>))
  {
    silva::env_context_t env_context_environ;
    silva::env_context_fill_environ(&env_context_environ);
    silva::env_context_t env_context_cmdline;
    silva::env_context_fill_cmdline(&env_context_cmdline, argc, argv);

    array_t<string_view_t> cmdline_args;
    for (int i = 0; i < argc; ++i) {
      const string_view_t arg_str{argv[i]};
      if (!arg_str.starts_with("--")) {
        cmdline_args.push_back(arg_str);
      }
    }
    const silva::expected_t<void> result = (*real_main)(cmdline_args);

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
