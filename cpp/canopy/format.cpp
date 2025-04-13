#include "format.hpp"

#include <fmt/format.h>

namespace silva {
  string_t format_vector(const span_t<string_view_t> args)
  {
    if (args.empty()) {
      return {};
    }
    using ctx = fmt::format_context;
    vector_t<fmt::basic_format_arg<ctx>> fmt_args;
    for (index_t i = 1; i < args.size(); ++i) {
      fmt_args.push_back(fmt::detail::make_arg<ctx>(args[i]));
    }
    return fmt::vformat(args.front(),
                        fmt::basic_format_args<ctx>(fmt_args.data(), fmt_args.size()));
  }
}
