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
  memento_item_type_t memento_item_type_custom();

  // A "memento-item" is a type-erased value that supports the following operations:
  //    * Conversion to a string_or_view_t
  //    * Move constructor
  //    * Destructor
  // A "memento" is a list of memento-items. A "memento-buffer" is a list of mementos.
  //
  // A memento-item can only contain of value of the types specified by "memento_item_type_t". This
  // can either be a native type (cf. definition of "memento_item_type_t") or a user-defined type
  // (cf. "memento_item_type_custom()"). See the "Widget" class in the "memento.tpp" file for an
  // example of how to enable custom class for mementoization.

  struct memento_item_ptr_t {
    const byte_t* ptr = nullptr;

    index_t size() const;
    memento_item_type_t type() const;

    string_or_view_t to_string_or_view() const;
  };

  //  - 4 bytes: total size (including this field)
  //  - 4 bytes: number of items
  //  - memento_items (the first one is always expected to be a format-string):
  //    - 4 bytes: total size of item (including this field)
  //    - 4 bytes: memento_item_type_t
  //    - payload
  struct memento_ptr_t {
    const byte_t* ptr = nullptr;

    index_t size() const;
    index_t num_items() const;

    memento_item_ptr_t at_offset(index_t) const;

    string_or_view_t to_string_or_view() const;

    template<typename Visitor>
      requires std::invocable<Visitor, index_t, const memento_item_ptr_t&>
    void for_each_item(Visitor) const;
  };

  class memento_buffer_index_t {
    friend struct memento_buffer_t;
    index_t index = 0;
    memento_buffer_index_t(index_t);

   public:
    memento_buffer_index_t() = default;

    friend auto operator<=>(const memento_buffer_index_t&, const memento_buffer_index_t&) = default;
    friend hash_value_t hash_impl(const memento_buffer_index_t& x);
  };

  struct memento_buffer_t {
    string_t buffer;

    memento_ptr_t at(memento_buffer_index_t) const;

    void resize(memento_buffer_index_t);

    memento_buffer_index_t push_back_materialized(const memento_ptr_t&);

    template<typename... Args>
    memento_buffer_index_t push_back(const Args&...);

    template<typename Visitor>
      requires std::invocable<Visitor, memento_buffer_index_t, const memento_ptr_t&>
    void for_each_memento(Visitor) const;
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
}

// IMPLEMENTATION

namespace silva {
  inline memento_item_type_t memento_item_type_custom()
  {
    using enum memento_item_type_t;
    static uint32_t next_custom = std::to_underlying(CUSTOM_BEGIN);
    next_custom += 1;
    return static_cast<memento_item_type_t>(next_custom);
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
    static memento_item_type_t write(string_t& buffer, const string_view_t& x)
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
      using TT                      = std::decay_t<decltype(x)>;
      const memento_item_type_t mit = memento_item_writer_t<TT>::write(buffer, x);
      const index_t new_size        = buffer.size();
      const index_t total_length    = new_size - old_size;
      bit_write_at<uint32_t>(buffer.data() + old_size, total_length);
      bit_write_at<uint32_t>(buffer.data() + old_size + 4, static_cast<uint32_t>(mit));
      return total_length;
    }
  }

  template<typename... Args>
  memento_buffer_index_t memento_buffer_t::push_back(const Args&... args)
  {
    const index_t retval = buffer.size();
    bit_append<uint32_t>(buffer, 0); // placeholder for total_size
    bit_append<uint32_t>(buffer, sizeof...(Args));
    index_t total_size = 4 + 4;
    ((total_size += impl::memento_item_write<Args>(buffer, args)), ...);
    bit_write_at<uint32_t>(buffer.data() + retval, total_size);
    return memento_buffer_index_t{retval};
  }

  template<typename Visitor>
    requires std::invocable<Visitor, index_t, const memento_item_ptr_t&>
  void memento_ptr_t::for_each_item(Visitor visitor) const
  {
    const index_t ss = size();
    index_t offset   = 8;
    while (offset < ss) {
      memento_item_ptr_t memento_item = at_offset(offset);
      visitor(offset, memento_item);
      offset += memento_item.size();
    }
  }

  template<typename Visitor>
    requires std::invocable<Visitor, memento_buffer_index_t, const memento_ptr_t&>
  void memento_buffer_t::for_each_memento(Visitor visitor) const
  {
    memento_buffer_index_t offset{0};
    while (offset.index < buffer.size()) {
      memento_ptr_t memento = at(offset);
      visitor(offset, memento);
      offset.index += memento.size();
    }
  }
}
