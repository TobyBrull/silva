#include "stream.hpp"

#include <iostream>

namespace silva {
  // stream_t

  void stream_t::write(span_t<const byte_t> data)
  {
    bool first = true;
    while (!data.empty()) {
      if (!first) {
        flush(data.size());
      }
      const index_t to_write = std::min(data.size(), target.size());
      std::copy(data.begin(), data.begin() + to_write, target.begin());
      target = target.subspan(to_write);
      data   = data.subspan(to_write);
      first  = false;
    }
  }

  void stream_t::write_str(string_view_t data)
  {
    return write(span_t<const byte_t>((const byte_t*)data.data(), data.size()));
  }

  // stream_stdout_t

  stream_stdout_t::stream_stdout_t(const index_t init_buffer_size) : buffer(init_buffer_size)
  {
    target = buffer;
  }
  stream_stdout_t::~stream_stdout_t()
  {
    flush();
  }

  void stream_stdout_t::flush(const index_t next_write_hint)
  {
    const index_t curr_used = buffer.size() - target.size();
    const string_view_t data((const char*)buffer.data(), curr_used);
    std::cout << data << std::flush;
    buffer.resize(std::max<index_t>(next_write_hint, buffer.size()));
    target = buffer;
  }

  // stream_memory_t

  stream_memory_t::stream_memory_t(const index_t init_buffer_size) : buffer(init_buffer_size)
  {
    target = buffer;
  }

  index_t _mem_new_size(index_t curr_size, const index_t curr_used, const index_t next_write_hint)
  {
    const index_t min_req_size = curr_used + std::max(1, next_write_hint);
    while (curr_size < min_req_size) {
      curr_size *= 2;
    }
    return curr_size;
  }

  void stream_memory_t::clear()
  {
    target = buffer;
  }

  span_t<const byte_t> stream_memory_t::content() const
  {
    const index_t curr_used = buffer.size() - target.size();
    return span_t<const byte_t>(buffer).subspan(0, curr_used);
  }
  vector_t<byte_t> stream_memory_t::content_fetch()
  {
    const auto range = content();
    vector_t<byte_t> retval{range.begin(), range.end()};
    clear();
    return retval;
  }

  string_view_t stream_memory_t::content_str() const
  {
    const auto retval = content();
    return string_view_t{(const char*)retval.data(), retval.size()};
  }
  string_t stream_memory_t::content_str_fetch()
  {
    string_t retval{content_str()};
    clear();
    return retval;
  }

  void stream_memory_t::flush(const index_t next_write_hint)
  {
    const index_t curr_used = buffer.size() - target.size();
    const index_t new_size  = _mem_new_size(buffer.size(), curr_used, next_write_hint);
    buffer.resize(new_size);
    target = span_t<byte_t>(buffer).subspan(curr_used);
  }
}
