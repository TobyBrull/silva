#include "assert.hpp"

namespace silva::detail {
  [[noreturn]] void
  assert_handler(char const* file, long const line, char const* func, const string_t& error_message)
  {
    fmt::print(stderr,
               FMT_STRING("\n\nsilva: internal error in {} at {}:{}\nERROR:\n{}\n\n"),
               func,
               file,
               line,
               error_message);
    std::abort();
  }
}
