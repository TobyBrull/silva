#include "any_vector.hpp"

#include <catch2/catch_all.hpp>

#include <fmt/format.h>

namespace silva {
  string_t my_string_impl(const string_t& x)
  {
    return x;
  }

  string_t my_string_impl(const int& x)
  {
    return std::to_string(x);
  }

  string_t my_string_impl(const tuple_t<int, int, int>& x)
  {
    return fmt::format("[{},{},{}]", std::get<0>(x), std::get<1>(x), std::get<2>(x));
  }

  struct my_string_t : public customization_point_t<string_t(const void*)> {
    template<typename T>
    constexpr string_t operator()(const T& x) const
    {
      using silva::my_string_impl;
      return my_string_impl(x);
    }
  };
  inline constexpr my_string_t my_string;

  TEST_CASE("any_vector", "[any_vector_t]")
  {
    any_vector_t<hash_t, my_string_t, move_ctor_t, dtor_t> av(64);
    const auto i1 = av.push_back(string_t{"Hello"});
    const auto i2 = av.push_back(314);
    const auto i3 = av.push_back(tuple_t<int, int, int>{5, 1, 3});
    CHECK(av.apply(i1, my_string) == "Hello");
    CHECK(av.apply(i2, my_string) == "314");
    CHECK(av.apply(i3, my_string) == "[5,1,3]");
    CHECK(av.size() == 3);
    CHECK(av.byte_size() ==
          8 + chunked_size(sizeof(string_t), av.alignment) + 8 +
              chunked_size(sizeof(int), av.alignment) + 8 +
              chunked_size(sizeof(tuple_t<int, int, int>), av.alignment));
    av.resize_down_to(i2);
    CHECK(av.size() == 1);
    CHECK(av.byte_size() == 8 + chunked_size(sizeof(string_t), av.alignment));
    CHECK(av.apply(i1, my_string) == "Hello");
  }
}
