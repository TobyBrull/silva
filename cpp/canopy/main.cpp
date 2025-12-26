#include "main.hpp"

#include "var_context.hpp"

namespace silva {
  int main(const int argc, char* argv[], expected_t<void> (*real_main)(span_t<string_view_t>))
  {
    silva::var_context_t var_context_environ;
    silva::var_context_fill_environ(&var_context_environ);
    silva::var_context_t var_context_cmdline;
    silva::var_context_fill_cmdline(&var_context_cmdline, argc, argv);

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
      fmt::print(stderr, "ERROR ({}):\n{}\n", pretty_string(error.level), pretty_string(error));
      return static_cast<int>(error.level);
    }
    else {
      return 0;
    }
  }
}
