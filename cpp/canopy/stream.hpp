#pragma once

#include "types.hpp"

#include <fmt/format.h>

namespace silva {
  struct stream_t {
    span_t<byte_t> target;

    void write(span_t<const byte_t>);
    void write_str(string_view_t);

    template<typename... T>
    void format(fmt::format_string<T...> fmt, T&&... args);

    virtual void flush(index_t next_write_hint = 0) = 0;
  };

  struct stream_stdout_t : public stream_t {
    vector_t<byte_t> buffer;

    stream_stdout_t(index_t init_buffer_size = 4 * 1'024);
    ~stream_stdout_t();

    void flush(index_t = 0) final;
  };

  struct stream_memory_t : public stream_t {
    vector_t<byte_t> buffer;

    stream_memory_t(index_t init_buffer_size = 32);

    void clear();

    span_t<const byte_t> content() const;
    vector_t<byte_t> content_fetch();

    string_view_t content_str() const;
    string_t content_str_fetch();

    void flush(index_t = 0) final;
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename... Args>
  void stream_t::format(fmt::format_string<Args...> fmt, Args&&... args)
  {
    const auto temp = fmt::format(fmt, std::forward<Args>(args)...);
    write_str(temp);
  }
}
