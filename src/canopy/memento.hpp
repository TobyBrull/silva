#pragma once

#include "assert.hpp"
#include "string_or_view.hpp"
#include "types.hpp"
#include <cstring>

namespace silva {
  enum class memento_item_type_t {
    INVALID = 0,
    STRING_VIEW,
    STRING,
    INTEGER_32,
    INTEGER_64,
    DOUBLE_32,
    DOUBLE_64,
    BOOLEAN,

    CUSTOM_BEGIN = 0x1000,
  };

  struct memento_item_t {
    const byte_t* ptr = nullptr;

    memento_item_type_t type() const;

    string_or_view_t to_string_or_view() const;
  };

  struct memento_t {
    const byte_t* ptr = nullptr;

    index_t num_items() const;

    string_or_view_t to_string_or_view() const;
  };

  using memento_buffer_offset_t = index_t;

  // One memento:
  //
  //  - 4 bytes: total size (including this field)
  //  - 4 bytes: number of items
  //  - memento_items:
  //    - 4 bytes: total size of item (including this field)
  //    - 4 bytes: memento_item_type_t
  //    - payload
  //
  struct memento_buffer_t {
    string_t buffer;

    memento_t at_offset(memento_buffer_offset_t) const;

    void resize_offset(memento_buffer_offset_t);

    template<typename... Ts>
    memento_buffer_offset_t append_memento(string_or_view_t format_str, const Ts&...);
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
  }

  template<typename... Ts>
  memento_buffer_offset_t memento_buffer_t::append_memento(string_or_view_t format_str,
                                                           const Ts&...)
  {
    static_assert(sizeof...(Ts) == 0, "Not implemented yet");
    const index_t retval                   = buffer.size();
    variant_t<string_view_t, string_t>& vv = format_str.data;
    if (vv.index() == 0) {
      const string_view_t& x   = std::get<string_view_t>(vv);
      const index_t total_size = 4 + 4 + (4 + 4 + 8 + 8);
      impl::push_bitwise<uint32_t>(buffer, total_size);
      impl::push_bitwise<uint32_t>(buffer, 1);
      impl::push_bitwise<uint32_t>(buffer, 24);
      impl::push_bitwise<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::STRING_VIEW));
      impl::push_bitwise<const void*>(buffer, x.data());
      impl::push_bitwise<uint64_t>(buffer, x.size());
    }
    else if (vv.index() == 1) {
      const string_t& x        = std::get<string_t>(vv);
      const index_t total_size = 4 + 4 + (4 + 4 + x.size());
      impl::push_bitwise<uint32_t>(buffer, total_size);
      impl::push_bitwise<uint32_t>(buffer, 1);
      impl::push_bitwise<uint32_t>(buffer, total_size - 8);
      impl::push_bitwise<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::STRING));
      const index_t old_size = buffer.size();
      buffer.resize(old_size + x.size());
      memcpy(buffer.data() + old_size, x.data(), x.size());
    }
    else {
      SILVA_ASSERT(false);
    }
    return retval;
  }
}
