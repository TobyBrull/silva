#include "memento.hpp"

#include "preprocessor.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("memento", "[memento_t]")
  {
    memento_buffer_t mb;
    const auto i1 = mb.push_back(string_view_t{"Hello World"});
    const auto i2 = mb.push_back(string_t{"Hello Silva"});
    const auto i3 = mb.push_back(string_view_t{"Hello all"});

    CHECK(mb.at(i1).num_items() == 1);
    CHECK(mb.at(i1).to_string_or_view().get_view() == "Hello World");
    CHECK(mb.at(i2).num_items() == 1);
    CHECK(mb.at(i2).to_string_or_view().get_view() == "Hello Silva");
    CHECK(mb.at(i3).num_items() == 1);
    CHECK(mb.at(i3).to_string_or_view().get_view() == "Hello all");

    mb.resize(i2);

    CHECK(mb.at(i1).num_items() == 1);
    CHECK(mb.at(i1).to_string_or_view().get_view() == "Hello World");
  }

  TEST_CASE("memento variadic", "[memento_t]")
  {
    memento_buffer_t mb;
    const auto i1 = mb.push_back(string_view_t{"Hello {} World {}"}, 42, 3.14159);
    const auto i2 = mb.push_back(string_t{"Hello World War {}"}, false);

    CHECK(mb.at(i1).to_string_or_view().get_view() == "Hello 42 World 3.141590");
    CHECK(mb.at(i2).to_string_or_view().get_view() == "Hello World War false");
  }

  struct Widget {
    int x = 0;
  };

  TEST_CASE("memento customization", "[memento_t]")
  {
    memento_buffer_t mb;
    const auto i1 = mb.push_back("Hello {}", Widget{.x = 42});
    CHECK(mb.at(i1).to_string_or_view().get_view() == "Hello Widget(42)");
  }
}

template<>
struct silva::memento_item_writer_t<silva::test::Widget> {
  inline static memento_item_type_t memento_item_type = memento_item_type_custom();
  static memento_item_type_t write(string_t& buffer, const silva::test::Widget& x)
  {
    bit_append<silva::test::Widget>(buffer, x);
    return memento_item_type;
  }
  static inline SILVA_USED bool reg = memento_item_reader_t::register_reader(
      memento_item_type,
      [](const byte_t* ptr, const index_t size) -> string_or_view_t {
        SILVA_ASSERT(size == sizeof(silva::test::Widget));
        const silva::test::Widget ww = bit_cast_ptr<silva::test::Widget>(ptr);
        return string_or_view_t{fmt::format("Widget({})", ww.x)};
      });
};
