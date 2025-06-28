#include "byte_sink.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("stream_stdout_t")
  {
    byte_sink_stdout_t stream;
    stream.format("Hello {}!\n", "Silva");
    stream.on_out_of_span();
  }
  TEST_CASE("stream_memory_t")
  {
    for (const index_t init_buf_size: {1, 4, 128}) {
      byte_sink_memory_t stream(init_buf_size);
      stream.format("Hello {} {}", 42, "World\n");
      const string_t test = "Test\n";
      stream.write(span_t<const byte_t>((const byte_t*)test.data(), test.size()));
      stream.on_out_of_span();
      CHECK(stream.content_str() == "Hello 42 World\nTest\n");
    }
  }
}
