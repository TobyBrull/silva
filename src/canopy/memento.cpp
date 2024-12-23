#include "memento.hpp"
#include <cstring>

namespace silva {
  namespace impl {
    template<typename T>
    T ptr_bit_cast(const void* ptr)
    {
      T retval;
      memcpy(&retval, ptr, sizeof(T));
      return retval;
    }
  }

  memento_item_type_t memento_item_t::type() const
  {
    const uint32_t type = impl::ptr_bit_cast<uint32_t>(ptr + 4);
    return static_cast<memento_item_type_t>(type);
  }

  string_or_view_t memento_item_t::to_string_or_view() const
  {
    const uint32_t size = impl::ptr_bit_cast<uint32_t>(ptr);
    SILVA_ASSERT(size >= 8);
    const memento_item_type_t tt = type();

    using enum memento_item_type_t;
    if (tt == STRING_VIEW) {
      SILVA_ASSERT(size == 24);
      const char* msg_ptr = impl::ptr_bit_cast<const char*>(ptr + 8);
      const auto msg_size = impl::ptr_bit_cast<uint64_t>(ptr + 16);
      return string_or_view_t{string_view_t(msg_ptr, msg_size)};
    }
    else if (tt == STRING) {
      return string_or_view_t{string_view_t(reinterpret_cast<const char*>(ptr + 8), size - 8)};
    }
    else {
      SILVA_ASSERT(false);
    }
  }

  index_t memento_t::num_items() const
  {
    const uint32_t retval = impl::ptr_bit_cast<uint32_t>(ptr + 4);
    return retval;
  }

  string_or_view_t memento_t::to_string_or_view() const
  {
    SILVA_ASSERT(num_items() == 1);
    memento_item_t item{.ptr = ptr + 8};
    return item.to_string_or_view();
  }

  memento_t memento_buffer_t::at_offset(const memento_buffer_offset_t offset) const
  {
    SILVA_ASSERT(offset + 8 <= buffer.size());
    const uint32_t total_size = impl::ptr_bit_cast<uint32_t>(buffer.data() + offset);
    const uint32_t num_items  = impl::ptr_bit_cast<uint32_t>(buffer.data() + offset + 4);
    SILVA_ASSERT(num_items == 1);
    return memento_t{.ptr = reinterpret_cast<const byte_t*>(buffer.data() + offset)};
  }

  void memento_buffer_t::resize_offset(const memento_buffer_offset_t new_size)
  {
    buffer.resize(new_size);
  }
}
