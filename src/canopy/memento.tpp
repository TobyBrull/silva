#include "memento.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("memento", "[memento_t]")
{
  memento_buffer_t mb;
  const auto i1 = mb.append_memento(string_or_view_t{string_view_t{"Hello World"}});
  const auto i2 = mb.append_memento(string_or_view_t{string_t{"Hello Silva"}});
  const auto i3 = mb.append_memento(string_or_view_t{string_view_t{"Hello all"}});

  CHECK(mb.at_offset(i1).num_items() == 1);
  CHECK(mb.at_offset(i1).to_string_or_view().get_view() == "Hello World");
  CHECK(mb.at_offset(i2).num_items() == 1);
  CHECK(mb.at_offset(i2).to_string_or_view().get_view() == "Hello Silva");
  CHECK(mb.at_offset(i3).num_items() == 1);
  CHECK(mb.at_offset(i3).to_string_or_view().get_view() == "Hello all");

  mb.resize_offset(i2);

  CHECK(mb.at_offset(i1).num_items() == 1);
  CHECK(mb.at_offset(i1).to_string_or_view().get_view() == "Hello World");
}
