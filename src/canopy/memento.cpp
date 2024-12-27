#include "memento.hpp"

namespace silva {
  memento_item_type_t memento_item_t::type() const
  {
    const uint32_t type = bit_cast_ptr<uint32_t>(ptr + 4);
    return static_cast<memento_item_type_t>(type);
  }

  index_t memento_item_t::size() const
  {
    const uint32_t type = bit_cast_ptr<uint32_t>(ptr);
    return static_cast<index_t>(type);
  }

  string_or_view_t memento_item_t::to_string_or_view() const
  {
    const index_t ss             = size();
    const memento_item_type_t tt = type();
    using enum memento_item_type_t;
    return memento_item_reader_t::apply(tt, ptr, ss);
  }

  static hashmap_t<std::underlying_type_t<memento_item_type_t>, memento_item_reader_t::callback_t>
      memento_item_reader_map = [] {
        hashmap_t<std::underlying_type_t<memento_item_type_t>, memento_item_reader_t::callback_t>
            retval;
        using enum memento_item_type_t;
        retval[std::to_underlying(STRING_VIEW)] = [](const byte_t* ptr,
                                                     const index_t size) -> string_or_view_t {
          SILVA_ASSERT(size == 24);
          const char* msg_ptr = bit_cast_ptr<const char*>(ptr + 8);
          const auto msg_size = bit_cast_ptr<uint64_t>(ptr + 16);
          return string_or_view_t{string_view_t(msg_ptr, msg_size)};
        };
        retval[std::to_underlying(STRING)] = [](const byte_t* ptr,
                                                const index_t size) -> string_or_view_t {
          return string_or_view_t{string_view_t(reinterpret_cast<const char*>(ptr + 8), size - 8)};
        };
        retval[std::to_underlying(BOOLEAN)] = [](const byte_t* ptr, const index_t size) {
          SILVA_ASSERT(size == 12);
          const auto val = bit_cast_ptr<uint32_t>(ptr + 8);
          return string_or_view_t{val == 0 ? string_view_t{"false"} : string_view_t{"true"}};
        };
        retval[std::to_underlying(INTEGER_64)] = [](const byte_t* ptr, const index_t size) {
          SILVA_ASSERT(size == 16);
          return string_or_view_t{std::to_string(bit_cast_ptr<int64_t>(ptr + 8))};
        };
        retval[std::to_underlying(FLOAT_64)] = [](const byte_t* ptr, const index_t size) {
          SILVA_ASSERT(size == 16);
          return string_or_view_t{std::to_string(bit_cast_ptr<double>(ptr + 8))};
        };
        return retval;
      }();

  string_or_view_t memento_item_reader_t::apply(const memento_item_type_t memento_item_type,
                                                const byte_t* ptr,
                                                const index_t size)
  {
    const auto it = memento_item_reader_map.find(std::to_underlying(memento_item_type));
    if (it != memento_item_reader_map.end()) {
      return it->second(ptr, size);
    }
    SILVA_ASSERT(false, "Unkown memento-type {}", std::to_underlying(memento_item_type));
  }

  index_t memento_t::num_items() const
  {
    const uint32_t retval = bit_cast_ptr<uint32_t>(ptr + 4);
    return retval;
  }

  namespace impl {
    string_t format_vector(const string_view_t format, const vector_t<string_t>& args)
    {
      using ctx = fmt::format_context;
      std::vector<fmt::basic_format_arg<ctx>> fmt_args;
      for (auto const& a: args) {
        fmt_args.push_back(fmt::detail::make_arg<ctx>(a));
      }
      return fmt::vformat(format, fmt::basic_format_args<ctx>(fmt_args.data(), fmt_args.size()));
    }
  }

  string_or_view_t memento_t::to_string_or_view() const
  {
    const uint32_t total_size = bit_cast_ptr<uint32_t>(ptr);
    const uint32_t num_items  = bit_cast_ptr<uint32_t>(ptr + 4);
    if (num_items == 1) {
      memento_item_t item{.ptr = ptr + 8};
      return item.to_string_or_view();
    }
    else {
      SILVA_ASSERT(num_items > 1);
      index_t offset = 8;
      vector_t<string_t> args;
      while (offset < total_size) {
        memento_item_t item{.ptr = ptr + offset};
        args.push_back(string_t{item.to_string_or_view().get_view()});
        offset += item.size();
      }
      string_t format = args.front();
      args.erase(args.begin());
      return string_or_view_t{impl::format_vector(format, args)};
    }
  }

  memento_t memento_buffer_t::at_offset(const memento_buffer_offset_t offset) const
  {
    return memento_t{.ptr = reinterpret_cast<const byte_t*>(buffer.data() + offset)};
  }

  void memento_buffer_t::resize_offset(const memento_buffer_offset_t new_size)
  {
    buffer.resize(new_size);
  }
}
