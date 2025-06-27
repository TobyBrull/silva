#pragma once

#include "types.hpp"

#include <fmt/format.h>

namespace silva {
  struct stream_out_t {
    span_t<byte_t> target;

    void write(span_t<const byte_t>);
    void write_str(string_view_t);

    template<typename... T>
    void format(fmt::format_string<T...> fmt, T&&... args);

    virtual void flush(index_t next_write_hint = 0) = 0;
  };

  struct stream_out_std_t : public stream_out_t {
    vector_t<byte_t> buffer;

    stream_out_std_t(index_t init_buffer_size = 4 * 1'024);

    void flush(index_t = 0) final;
  };

  struct stream_out_mem_t : public stream_out_t {
    vector_t<byte_t> buffer;

    stream_out_mem_t(index_t init_buffer_size = 4 * 1'024);

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
  void stream_out_t::format(fmt::format_string<Args...> fmt, Args&&... args)
  {
    const auto temp = fmt::format(fmt, std::forward<Args>(args)...);
    write_str(temp);
  }
}
