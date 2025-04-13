#include "format.hpp"

namespace silva {
  string_t format_vector(const string_view_t format, const span_t<string_view_t> args)
  {
    using ctx = fmt::format_context;
    std::vector<fmt::basic_format_arg<ctx>> fmt_args;
    for (auto const& a: args) {
      fmt_args.push_back(fmt::detail::make_arg<ctx>(a));
    }
    return fmt::vformat(format, fmt::basic_format_args<ctx>(fmt_args.data(), fmt_args.size()));
  }
}
