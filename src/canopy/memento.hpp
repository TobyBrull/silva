#pragma once

#include "assert.hpp"
#include "string_or_view.hpp"
#include "types.hpp"

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
  namespace impl {
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

    inline index_t push_memento_item_string_view(string_t& buffer, const string_view_t& x)
    {
      impl::push_bitwise<uint32_t>(buffer, 24);
      impl::push_bitwise<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::STRING_VIEW));
      impl::push_bitwise<const void*>(buffer, x.data());
      impl::push_bitwise<uint64_t>(buffer, x.size());
      return 24;
    }

    inline index_t push_memento_item_string(string_t& buffer, const string_t& x)
    {
      impl::push_bitwise<uint32_t>(buffer, 4 + 4 + x.size());
      impl::push_bitwise<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::STRING));
      const index_t old_size = buffer.size();
      buffer.resize(old_size + x.size());
      memcpy(buffer.data() + old_size, x.data(), x.size());
      return 4 + 4 + x.size();
    }

    inline index_t push_memento_item_integer(string_t& buffer, const uint64_t x)
    {
      impl::push_bitwise<uint32_t>(buffer, 4 + 4 + 8);
      impl::push_bitwise<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::INTEGER_64));
      impl::push_bitwise<uint64_t>(buffer, x);
      return 4 + 4 + 8;
    }

    inline index_t push_memento_item_double(string_t& buffer, const double x)
    {
      impl::push_bitwise<uint32_t>(buffer, 4 + 4 + 8);
      impl::push_bitwise<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::FLOAT_64));
      impl::push_bitwise<double>(buffer, x);
      return 4 + 4 + 8;
    }

    inline index_t push_memento_item_bool(string_t& buffer, const bool x)
    {
      impl::push_bitwise<uint32_t>(buffer, 4 + 4 + 4);
      impl::push_bitwise<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::BOOLEAN));
      impl::push_bitwise<double>(buffer, x);
      return 4 + 4 + 4;
    }

    template<typename T>
    index_t push_memento_item(string_t& buffer, const T& x)
    {
      using enum memento_item_type_t;
      if constexpr (std::same_as<T, string_view_t>) {
        return push_memento_item_string_view(buffer, x);
      }
      else if constexpr (std::same_as<std::decay_t<T>, char*>) {
        return push_memento_item_string_view(buffer, string_view_t{x});
      }
      else if constexpr (std::same_as<T, string_t>) {
        return push_memento_item_string(buffer, x);
      }
      else if constexpr (std::same_as<T, string_or_view_t>) {
        if (std::holds_alternative<string_view_t>(x.data)) {
          return push_memento_item_string_view(buffer, std::get<string_view_t>(x.data));
        }
        else {
          SILVA_ASSERT(std::holds_alternative<string_t>(x.data));
          return push_memento_item_string(buffer, std::get<string_t>(x.data));
        }
      }
      else if constexpr (std::same_as<T, bool>) {
        return push_memento_item_bool(buffer, x);
      }
      else if constexpr (std::integral<T>) {
        return push_memento_item_integer(buffer, x);
      }
      else if constexpr (std::floating_point<T>) {
        return push_memento_item_double(buffer, x);
      }
      else {
        static_assert(false, "Unknown memento type");
      }
    }
  }

  template<typename... Ts>
  memento_buffer_offset_t memento_buffer_t::append_memento(const Ts&... args)
  {
    const index_t retval = buffer.size();
    impl::push_bitwise<uint32_t>(buffer, 0); // placeholder for total_size
    impl::push_bitwise<uint32_t>(buffer, sizeof...(Ts));
    index_t total_size = 4 + 4;

    ((total_size += impl::push_memento_item(buffer, args)), ...);

    impl::overwrite_bits<uint32_t>(buffer.data() + retval, total_size);

    return retval;
  }
}
