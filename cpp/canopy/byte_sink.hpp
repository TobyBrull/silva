#pragma once

#include "string.hpp"

#include <fmt/format.h>

namespace silva {
  struct byte_sink_t {
    span_t<byte_t> span;

    void write(span_t<const byte_t>);
    void write_str(string_view_t);

    template<typename... T>
    void format(fmt::format_string<T...> fmt, T&&... args);

    static constexpr index_t min_buffer_size = 64;

    virtual void on_out_of_span(index_t size_hint = 0) = 0;
  };

  struct byte_sink_stdout_t : public byte_sink_t {
    array_t<byte_t> buffer;

    byte_sink_stdout_t(index_t init_buffer_size = min_buffer_size);
    ~byte_sink_stdout_t();

    void on_out_of_span(index_t = 0) final;
  };

  struct byte_sink_memory_t : public byte_sink_t {
    array_t<byte_t> buffer;

    byte_sink_memory_t(index_t init_buffer_size = min_buffer_size);

    void clear();

    span_t<const byte_t> content() const;
    array_t<byte_t> content_fetch();

    string_view_t content_str() const;
    string_t content_str_fetch();

    void on_out_of_span(index_t = 0) final;
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename... Args>
  void byte_sink_t::format(fmt::format_string<Args...> fmt, Args&&... args)
  {
    const auto temp = fmt::format(fmt, std::forward<Args>(args)...);
    write_str(temp);
  }
}
