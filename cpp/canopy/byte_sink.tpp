#include "byte_sink.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("stream_stdout_t")
  {
    byte_sink_stdout_t byte_sink;
    byte_sink.format("Hello {}!\n", "Silva");
    byte_sink.on_out_of_span();
  }
  TEST_CASE("stream_memory_t")
  {
    for (const index_t init_buf_size: {1, 4, 128}) {
      byte_sink_memory_t byte_sink(init_buf_size);
      byte_sink.format("Hello {} {}", 42, "World\n");
      const string_t test = "Test\n";
      byte_sink.write(span_t<const byte_t>((const byte_t*)test.data(), test.size()));
      byte_sink.on_out_of_span();
      CHECK(byte_sink.content_str() == "Hello 42 World\nTest\n");
    }
  }
}
