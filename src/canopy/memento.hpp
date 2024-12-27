#pragma once

#include "assert.hpp"
#include "string_or_view.hpp"
#include "types.hpp"

#include <functional>

namespace silva {
  enum class memento_item_type_t {
    INVALID = 0,
    STRING_VIEW, // Two 8 byte pointers.
    STRING,      // Arbitrary size array of chars.
    BOOLEAN,     // 4 bytes.
    INTEGER_64,  // 8 bytes.
    FLOAT_64,    // 8 bytes.

    CUSTOM_BEGIN = 0x1000,
  };

  struct memento_item_t {
    const byte_t* ptr = nullptr;

    memento_item_type_t type() const;
    index_t size() const;

    string_or_view_t to_string_or_view() const;
  };

  template<typename T>
  struct memento_item_writer_t {
    static_assert(false, "memento_item_writer_t not specialised for this type");
  };

  struct memento_item_reader_t {
    using callback_t = std::function<string_or_view_t(const byte_t*, index_t)>;
    static void register_reader(memento_item_type_t, callback_t);
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

    template<typename... Ts>
    memento_buffer_offset_t append_memento(const Ts&...);
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  void push_bitwise(string_t& buffer, T x)
  {
    const index_t old_size = buffer.size();
    buffer.resize(old_size + sizeof(T));
    memcpy(buffer.data() + old_size, &x, sizeof(T));
  }

  template<typename T>
  T ptr_bit_cast(const void* ptr)
  {
    T retval;
    memcpy(&retval, ptr, sizeof(T));
    return retval;
  }

  template<typename T>
  void overwrite_bits(void* ptr, T x)
  {
    memcpy(ptr, &x, sizeof(T));
  }

  template<>
  struct memento_item_writer_t<string_view_t> {
    static index_t write(string_t& buffer, const string_view_t& x)
    {
      push_bitwise<uint32_t>(buffer, 24);
      push_bitwise<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::STRING_VIEW));
      push_bitwise<const void*>(buffer, x.data());
      push_bitwise<uint64_t>(buffer, x.size());
      return 24;
    }
  };

  template<>
  struct memento_item_writer_t<const char*> {
    static index_t write(string_t& buffer, const char* x)
    {
      return memento_item_writer_t<string_view_t>::write(buffer, string_view_t{x});
    }
  };

  template<>
  struct memento_item_writer_t<string_t> {
    using enum memento_item_type_t;
    constexpr inline static memento_item_type_t memento_item_type = STRING;
    static index_t write(string_t& buffer, const string_t& x)
    {
      push_bitwise<uint32_t>(buffer, 4 + 4 + x.size());
      push_bitwise<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::STRING));
      const index_t old_size = buffer.size();
      buffer.resize(old_size + x.size());
      memcpy(buffer.data() + old_size, x.data(), x.size());
      return 4 + 4 + x.size();
    }
  };

  template<>
  struct memento_item_writer_t<string_or_view_t> {
    static index_t write(string_t& buffer, const string_or_view_t& x)
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
    using enum memento_item_type_t;
    constexpr inline static memento_item_type_t memento_item_type = BOOLEAN;
    static index_t write(string_t& buffer, const bool x)
    {
      push_bitwise<uint32_t>(buffer, 4 + 4 + 4);
      push_bitwise<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::BOOLEAN));
      push_bitwise<double>(buffer, x);
      return 4 + 4 + 4;
    }
  };

  template<typename Integer>
    requires std::integral<Integer>
  struct memento_item_writer_t<Integer> {
    using enum memento_item_type_t;
    constexpr inline static memento_item_type_t memento_item_type = INTEGER_64;
    static index_t write(string_t& buffer, const int64_t x)
    {
      push_bitwise<uint32_t>(buffer, 4 + 4 + 8);
      push_bitwise<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::INTEGER_64));
      push_bitwise<int64_t>(buffer, x);
      return 4 + 4 + 8;
    }
  };

  template<typename Float>
    requires std::floating_point<Float>
  struct memento_item_writer_t<Float> {
    using enum memento_item_type_t;
    constexpr inline static memento_item_type_t memento_item_type = FLOAT_64;
    static index_t write(string_t& buffer, const double x)
    {
      push_bitwise<uint32_t>(buffer, 4 + 4 + 8);
      push_bitwise<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::FLOAT_64));
      push_bitwise<double>(buffer, x);
      return 4 + 4 + 8;
    }
  };

  template<typename... Ts>
  memento_buffer_offset_t memento_buffer_t::append_memento(const Ts&... args)
  {
    const index_t retval = buffer.size();
    push_bitwise<uint32_t>(buffer, 0); // placeholder for total_size
    push_bitwise<uint32_t>(buffer, sizeof...(Ts));
    index_t total_size = 4 + 4;

    ((total_size += memento_item_writer_t<std::decay_t<decltype(args)>>::write(buffer, args)), ...);

    overwrite_bits<uint32_t>(buffer.data() + retval, total_size);

    return retval;
  }
}
