#include "format.hpp"

#include <fmt/args.h>
#include <fmt/format.h>

namespace silva {
  string_t format_vector(const span_t<string_view_t> args)
  {
    if (args.empty()) {
      return {};
    }
    fmt::dynamic_format_arg_store<fmt::format_context> store;
    for (size_t i = 1; i < args.size(); ++i) {
      store.push_back(args[i]);
    }
    return fmt::vformat(args.front(), store);
  }

  string_t format_vector(const span_t<string_t> args)
  {
    vector_t<string_view_t> args_view;
    for (const auto& arg: args) {
      args_view.emplace_back(arg);
    }
    return string_t{format_vector(args_view)};
  }
}
