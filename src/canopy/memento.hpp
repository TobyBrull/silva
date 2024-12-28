#pragma once

#include "assert.hpp"
#include "bit.hpp"
#include "string_or_view.hpp"

#include <functional>
#include <utility>

namespace silva {
  enum class memento_item_type_t : uint32_t {
    INVALID = 0,
    STRING_VIEW, // Two 8 byte pointers.
    STRING,      // Arbitrary size array of chars.
    BOOLEAN,     // 4 bytes.
    INTEGER_64,  // 8 bytes.
    FLOAT_64,    // 8 bytes.

    CUSTOM_BEGIN = 0x1000,
  };
  constexpr memento_item_type_t memento_item_type_custom(index_t custom_begin_offset);

  struct memento_item_t {
    const byte_t* ptr = nullptr;

    memento_item_type_t type() const;
    index_t size() const;

    string_or_view_t to_string_or_view() const;
  };

  template<typename T>
  struct memento_item_writer_t {
    static_assert(false, "memento_item_writer_t not specialised for this type");

    static index_t write(string_t& buffer, const string_view_t& x);
  };

  struct memento_item_reader_t {
    using callback_t = std::function<string_or_view_t(const byte_t*, index_t)>;
    static bool register_reader(memento_item_type_t, callback_t);
    static string_or_view_t apply(memento_item_type_t, const byte_t*, index_t size);
  };

  //  - 4 bytes: total size (including this field)
  //  - 4 bytes: number of items
  //  - memento_items (the first one is always expected to be a format-string):
  //    - 4 bytes: total size of item (including this field)
  //    - 4 bytes: memento_item_type_t
  //    - payload
  struct memento_t {
    const byte_t* ptr = nullptr;

    index_t num_items() const;

    string_or_view_t to_string_or_view() const;
  };

  using memento_buffer_offset_t = index_t;

  struct memento_buffer_t {
    string_t buffer;

    memento_t at_offset(memento_buffer_offset_t) const;

    void resize_offset(memento_buffer_offset_t);

    template<typename... Args>
    memento_buffer_offset_t append_memento(const Args&...);
  };
}

// IMPLEMENTATION

namespace silva {
  constexpr memento_item_type_t memento_item_type_custom(const index_t custom_begin_offset)
  {
    using enum memento_item_type_t;
    const uint32_t custom_begin = std::to_underlying(CUSTOM_BEGIN);
    return static_cast<memento_item_type_t>(custom_begin + custom_begin_offset);
  }

  template<>
  struct memento_item_writer_t<string_view_t> {
    static memento_item_type_t write(string_t& buffer, const string_view_t& x)
    {
      bit_append<const void*>(buffer, x.data());
      bit_append<uint64_t>(buffer, x.size());
      return memento_item_type_t::STRING_VIEW;
    }
  };

  template<>
  struct memento_item_writer_t<const char*> {
    static memento_item_type_t write(string_t& buffer, const char* x)
    {
      return memento_item_writer_t<string_view_t>::write(buffer, string_view_t{x});
    }
  };

  template<>
  struct memento_item_writer_t<string_t> {
    static memento_item_type_t write(string_t& buffer, const string_t& x)
    {
      const index_t old_size = buffer.size();
      buffer.resize(old_size + x.size());
      memcpy(buffer.data() + old_size, x.data(), x.size());
      return memento_item_type_t::STRING;
    }
  };

  template<>
  struct memento_item_writer_t<string_or_view_t> {
    static memento_item_type_t write(string_t& buffer, const string_or_view_t& x)
    {
      if (std::holds_alternative<string_view_t>(x.data)) {
        return memento_item_writer_t<string_view_t>::write(buffer, std::get<string_view_t>(x.data));
      }
      else {
        SILVA_ASSERT(std::holds_alternative<string_t>(x.data));
        return memento_item_writer_t<string_t>::write(buffer, std::get<string_t>(x.data));
      }
    }
  };

  template<>
  struct memento_item_writer_t<bool> {
    static memento_item_type_t write(string_t& buffer, const bool x)
    {
      bit_append<uint32_t>(buffer, x);
      return memento_item_type_t::BOOLEAN;
    }
  };

  template<typename Integer>
    requires std::integral<Integer>
  struct memento_item_writer_t<Integer> {
    static memento_item_type_t write(string_t& buffer, const int64_t x)
    {
      bit_append<int64_t>(buffer, x);
      return memento_item_type_t::INTEGER_64;
    }
  };

  template<typename Float>
    requires std::floating_point<Float>
  struct memento_item_writer_t<Float> {
    static memento_item_type_t write(string_t& buffer, const double x)
    {
      bit_append<double>(buffer, x);
      return memento_item_type_t::FLOAT_64;
    }
  };

  namespace impl {
    template<typename T>
    index_t memento_item_write(string_t& buffer, const T& x)
    {
      const index_t old_size = buffer.size();
      bit_append<uint32_t>(buffer, 0); // placeholder for size
      bit_append<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::INVALID));
      const memento_item_type_t mit =
          memento_item_writer_t<std::decay_t<decltype(x)>>::write(buffer, x);
      const index_t new_size     = buffer.size();
      const index_t total_length = new_size - old_size;
      bit_write_at<uint32_t>(buffer.data() + old_size, total_length);
      bit_write_at<uint32_t>(buffer.data() + old_size + 4, static_cast<uint32_t>(mit));
      return total_length;
    }
  }

  template<typename... Args>
  memento_buffer_offset_t memento_buffer_t::append_memento(const Args&... args)
  {
    const index_t retval = buffer.size();
    bit_append<uint32_t>(buffer, 0); // placeholder for total_size
    bit_append<uint32_t>(buffer, sizeof...(Args));
    index_t total_size = 4 + 4;

    ((total_size += impl::memento_item_write<Args>(buffer, args)), ...);

    bit_write_at<uint32_t>(buffer.data() + retval, total_size);

    return retval;
  }
}
