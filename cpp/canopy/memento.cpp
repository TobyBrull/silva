#include "memento.hpp"

#include "format.hpp"
#include "preprocessor.hpp"

namespace silva {
  using enum memento_item_type_t;

  memento_item_type_t memento_item_ptr_t::type() const
  {
    const uint32_t type = bit_cast_ptr<uint32_t>(ptr + 4);
    return static_cast<memento_item_type_t>(type);
  }

  index_t memento_item_ptr_t::size() const
  {
    const uint32_t type = bit_cast_ptr<uint32_t>(ptr);
    return static_cast<index_t>(type);
  }

  string_or_view_t memento_item_ptr_t::to_string_or_view() const
  {
    const index_t ss             = size();
    const memento_item_type_t tt = type();
    return memento_item_reader_t::apply(tt, ptr, ss);
  }

  using memento_item_reader_map_t =
      hashmap_t<std::underlying_type_t<memento_item_type_t>, memento_item_reader_t::callback_t>;
  memento_item_reader_map_t& memento_item_reader_map()
  {
    static memento_item_reader_map_t memento_item_reader_map;
    return memento_item_reader_map;
  }

  static SILVA_USED bool reg_1 = memento_item_reader_t::register_reader(
      STRING_VIEW,
      [](const byte_t* ptr, const index_t size) -> string_or_view_t {
        SILVA_ASSERT(size == 16);
        const char* msg_ptr = bit_cast_ptr<const char*>(ptr);
        const auto msg_size = bit_cast_ptr<uint64_t>(ptr + 8);
        return string_or_view_t{string_view_t(msg_ptr, msg_size)};
      });
  static SILVA_USED bool reg_2 = memento_item_reader_t::register_reader(
      STRING,
      [](const byte_t* ptr, const index_t size) -> string_or_view_t {
        return string_or_view_t{string_view_t(reinterpret_cast<const char*>(ptr), size)};
      });
  static SILVA_USED bool reg_3 =
      memento_item_reader_t::register_reader(BOOLEAN, [](const byte_t* ptr, const index_t size) {
        SILVA_ASSERT(size == 4);
        const auto val = bit_cast_ptr<uint32_t>(ptr);
        return string_or_view_t{val == 0 ? string_view_t{"false"} : string_view_t{"true"}};
      });
  static SILVA_USED bool reg_4 =
      memento_item_reader_t::register_reader(INTEGER_64, [](const byte_t* ptr, const index_t size) {
        SILVA_ASSERT(size == 8);
        return string_or_view_t{std::to_string(bit_cast_ptr<int64_t>(ptr))};
      });
  static SILVA_USED bool reg_5 =
      memento_item_reader_t::register_reader(FLOAT_64, [](const byte_t* ptr, const index_t size) {
        SILVA_ASSERT(size == 8);
        return string_or_view_t{std::to_string(bit_cast_ptr<double>(ptr))};
      });

  bool memento_item_reader_t::register_reader(const memento_item_type_t mit, callback_t cb)
  {
    const auto [it, inserted] = memento_item_reader_map().emplace(std::to_underlying(mit), cb);
    SILVA_ASSERT(inserted,
                 "memento_item_type_t with value '{}' is re-used",
                 std::to_underlying(mit));
    return inserted;
  }

  string_or_view_t memento_item_reader_t::apply(const memento_item_type_t memento_item_type,
                                                const byte_t* ptr,
                                                const index_t size)
  {
    const auto it = memento_item_reader_map().find(std::to_underlying(memento_item_type));
    if (it != memento_item_reader_map().end()) {
      return it->second(ptr + 8, size - 8);
    }
    SILVA_ASSERT(false, "Unkown memento-type {}", std::to_underlying(memento_item_type));
  }

  index_t memento_ptr_t::size() const
  {
    const uint32_t retval = bit_cast_ptr<uint32_t>(ptr);
    return retval;
  }

  index_t memento_ptr_t::num_items() const
  {
    const uint32_t retval = bit_cast_ptr<uint32_t>(ptr + 4);
    return retval;
  }

  memento_item_ptr_t memento_ptr_t::at_offset(const index_t offset) const
  {
    return memento_item_ptr_t{.ptr = ptr + offset};
  }

  string_or_view_t memento_ptr_t::to_string_or_view() const
  {
    const uint32_t total_size = bit_cast_ptr<uint32_t>(ptr);
    const uint32_t num_items  = bit_cast_ptr<uint32_t>(ptr + 4);
    if (num_items == 1) {
      memento_item_ptr_t item{.ptr = ptr + 8};
      return item.to_string_or_view();
    }
    else {
      SILVA_ASSERT(num_items > 1);
      index_t offset = 8;
      vector_t<string_t> args;
      while (offset < total_size) {
        memento_item_ptr_t item{.ptr = ptr + offset};
        args.push_back(item.to_string_or_view().as_string());
        offset += item.size();
      }
      return string_or_view_t{format_vector(args)};
    }
  }

  memento_buffer_index_t::memento_buffer_index_t(const index_t index) : index(index) {}

  hash_value_t hash_impl(const memento_buffer_index_t& x)
  {
    return hash(x.index);
  }

  memento_buffer_index_t memento_buffer_t::push_back_materialized(const memento_ptr_t& memento)
  {
    const index_t retval = buffer.size();
    bit_append<uint32_t>(buffer, 0); // placeholder for total_size
    bit_append<uint32_t>(buffer, memento.num_items());
    memento.for_each_item([&](const index_t offset, const memento_item_ptr_t& item) {
      const index_t old_size = buffer.size();
      bit_append<uint32_t>(buffer, 0); // placeholder for size
      bit_append<uint32_t>(buffer, static_cast<uint32_t>(memento_item_type_t::INVALID));
      const memento_item_type_t mit =
          memento_item_writer_t<string_t>::write(buffer, item.to_string_or_view().as_string_view());
      SILVA_ASSERT(mit == memento_item_type_t::STRING);
      bit_write_at<uint32_t>(buffer.data() + old_size, buffer.size() - old_size);
      bit_write_at<uint32_t>(buffer.data() + old_size + 4, static_cast<uint32_t>(mit));
    });
    bit_write_at<uint32_t>(buffer.data() + retval, buffer.size() - retval);
    return memento_buffer_index_t{retval};
  }

  memento_ptr_t memento_buffer_t::at(const memento_buffer_index_t offset) const
  {
    return memento_ptr_t{.ptr = reinterpret_cast<const byte_t*>(buffer.data() + offset.index)};
  }

  void memento_buffer_t::resize(const memento_buffer_index_t new_size)
  {
    buffer.resize(new_size.index);
  }
}
