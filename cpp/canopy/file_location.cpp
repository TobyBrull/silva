#include "file_location.hpp"

namespace silva {
  void pretty_write_impl(const file_location_t& self, byte_sink_t* stream)
  {
    if (self == file_location_eof) {
      stream->format("EOF");
    }
    else {
      if (1) {
        stream->format("{}:{}", self.line_num + 1, self.column + 1);
      }
      else {
        stream->format("[{},{},{}]  ", self.line_num, self.column, self.byte_offset);
      }
    }
  }
}
