#pragma once

#include "byte_sink.hpp"
#include "types.hpp"

namespace silva {
  struct file_location_t {
    index_t line_num = 0;
    index_t column   = 0;

    friend auto operator<=>(const file_location_t&, const file_location_t&) = default;

    friend void pretty_write_impl(const file_location_t&, byte_sink_t*);
  };
  static constexpr file_location_t file_location_eof{.line_num = -1, .column = -1};
}
