#include "iterator_facade.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  struct range_iter_t : public iterator_facade_t {
    index_t pos = 0;

    auto dereference() const { return pos; }
    void increment() { pos += 1; }
    void decrement() { pos -= 1; }
    void advance(const index_t n) { pos += n; }
    index_t distance_to(const range_iter_t& other) { return other.pos - pos; }
    friend auto operator<=>(const range_iter_t&, const range_iter_t&) = default;
  };

  TEST_CASE("iterator_facade", "[iterator_facade_t]")
  {
    range_iter_t i1{.pos = 10};
    range_iter_t i2{.pos = 20};

    CHECK(*i1 == 10);
    CHECK(i2 - i1 == 10);
    CHECK(i1 < i2);
    CHECK(i1 <= i2);
    CHECK(i2 > i1);
    CHECK(i2 >= i1);
    CHECK(i1 == i1);
    CHECK(i2 != i1);
    CHECK(i1++ == range_iter_t{.pos = 10});
    CHECK(++i1 == range_iter_t{.pos = 12});
    CHECK(i2 - i1 == 8);
    i1 += 5;
    CHECK(i2 - i1 == 3);
    i1 -= 2;
    CHECK(i2 - i1 == 5);
    CHECK(i1 + 10 == range_iter_t{.pos = 25});
    CHECK(i1 - 10 == range_iter_t{.pos = 5});
  }
}
