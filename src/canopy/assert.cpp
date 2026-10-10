#include "assert.hpp"

#include <fmt/base.h>

namespace silva::impl {
  [[noreturn]] void assert_handler_core(char const* file,
                                        const long line,
                                        char const* func,
                                        const string_view_t error_message)
  {
    fmt::print(stderr,
               "\n\nsilva: internal error in {} at {}:{}\nERROR:\n{}\n\n",
               func,
               file,
               line,
               error_message);
    std::abort();
  }
}
