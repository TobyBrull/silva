#include "byte_sink.hpp"

#include <iostream>

namespace silva {
  // stream_t

  void byte_sink_t::write(span_t<const byte_t> data)
  {
    bool first = true;
    while (!data.empty()) {
      if (!first) {
        on_out_of_span(data.size());
      }
      const index_t to_write = std::min(data.size(), span.size());
      std::copy(data.begin(), data.begin() + to_write, span.begin());
      span  = span.subspan(to_write);
      data  = data.subspan(to_write);
      first = false;
    }
  }

  void byte_sink_t::write_str(string_view_t data)
  {
    return write(span_t<const byte_t>((const byte_t*)data.data(), data.size()));
  }

  // stream_stdout_t

  byte_sink_stdout_t::byte_sink_stdout_t(const index_t init_buffer_size) : buffer(init_buffer_size)
  {
    span = buffer;
  }
  byte_sink_stdout_t::~byte_sink_stdout_t()
  {
    on_out_of_span();
  }

  void byte_sink_stdout_t::on_out_of_span(const index_t size_hint)
  {
    const index_t curr_used = buffer.size() - span.size();
    const string_view_t data((const char*)buffer.data(), curr_used);
    std::cout << data << std::flush;
    buffer.resize(std::max<index_t>(size_hint, buffer.size()));
    span = buffer;
  }

  // stream_memory_t

  byte_sink_memory_t::byte_sink_memory_t(const index_t init_buffer_size) : buffer(init_buffer_size)
  {
    span = buffer;
  }

  index_t _mem_new_size(index_t curr_size, const index_t curr_used, const index_t size_hint)
  {
    const index_t min_req_size = curr_used + std::max<index_t>(1, size_hint);
    while (curr_size < min_req_size) {
      curr_size *= 2;
    }
    return curr_size;
  }

  void byte_sink_memory_t::clear()
  {
    span = buffer;
  }

  span_t<const byte_t> byte_sink_memory_t::content() const
  {
    const index_t curr_used = buffer.size() - span.size();
    return span_t<const byte_t>(buffer).subspan(0, curr_used);
  }
  vector_t<byte_t> byte_sink_memory_t::content_fetch()
  {
    const auto range = content();
    vector_t<byte_t> retval{range.begin(), range.end()};
    clear();
    return retval;
  }

  string_view_t byte_sink_memory_t::content_str() const
  {
    const auto retval = content();
    return string_view_t{(const char*)retval.data(), retval.size()};
  }
  string_t byte_sink_memory_t::content_str_fetch()
  {
    string_t retval{content_str()};
    clear();
    return retval;
  }

  void byte_sink_memory_t::on_out_of_span(const index_t size_hint)
  {
    const index_t curr_used = buffer.size() - span.size();
    const index_t new_size  = _mem_new_size(buffer.size(), curr_used, size_hint);
    buffer.resize(new_size);
    span = span_t<byte_t>(buffer).subspan(curr_used);
  }
}
