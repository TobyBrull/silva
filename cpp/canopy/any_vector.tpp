#include "any_vector.hpp"

#include "to_string.hpp"

#include <catch2/catch_all.hpp>

#include <fmt/format.h>

namespace silva::test {
  TEST_CASE("any_vector", "[any_vector_t]")
  {
    any_vector_t<hash_t, to_string_value_t, move_ctor_t, dtor_t> av(64);
    const auto i1 = av.push_back(string_t{"Hello"});
    const auto i2 = av.push_back(314);
    const auto i3 = av.push_back(tuple_t<int, int, int>{5, 1, 3});
    CHECK(av.apply(i1, to_string_value) == "Hello");
    CHECK(av.apply(i2, to_string_value) == "314");
    CHECK(av.apply(i3, to_string_value) == "[5 1 3]");
    CHECK(av.size() == 3);
    CHECK(av.byte_size() ==
          8 + chunked_size(sizeof(string_t), av.alignment) + 8 +
              chunked_size(sizeof(int), av.alignment) + 8 +
              chunked_size(sizeof(tuple_t<int, int, int>), av.alignment));
    av.resize_down_to(i2);
    CHECK(av.size() == 1);
    CHECK(av.byte_size() == 8 + chunked_size(sizeof(string_t), av.alignment));
    CHECK(av.apply(i1, to_string_value) == "Hello");
    const auto i4 = av.push_back(pair_t<int, float>{42, 1.5});
    const auto i5 = av.push_back(variant_t<pair_t<int, float>, float>{
        pair_t<int, float>{
            123,
            2.5,
        },
    });
    CHECK(av.size() == 3);
    CHECK(av.apply(i1, to_string_value) == "Hello");
    CHECK(av.apply(i4, to_string_value) == "[42 1.5]");
    CHECK(av.apply(i5, to_string_value) == "[123 2.5]");
    const auto i6 = av.push_back("World");
    CHECK(av.apply(i6, to_string_value) == "World");
  }
}
