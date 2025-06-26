#include "stream.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("stream_out_std_t")
  {
    stream_out_std_t out_std;
    out_std.format("Hello {}!\n", "Silva");
    out_std.flush();
  }
  TEST_CASE("stream_out_mem_t")
  {
    for (const index_t init_buf_size: {1, 4, 128}) {
      stream_out_mem_t out_mem(init_buf_size);
      out_mem.format("Hello {} {}", 42, "World\n");
      const string_t test = "Test\n";
      out_mem.write(span_t<const byte_t>((const byte_t*)test.data(), test.size()));
      out_mem.flush();
      CHECK(out_mem.content_str() == "Hello 42 World\nTest\n");
    }
  }
}
