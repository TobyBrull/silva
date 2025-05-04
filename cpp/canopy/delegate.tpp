#include "delegate.hpp"

#include <catch2/catch_all.hpp>

#include <fmt/format.h>

namespace silva::test {
  namespace {
    std::string free_func(const int i, const float f)
    {
      return fmt::format("free_func({}, {})", i, f);
    }

    std::string free_func_data(const std::string* s, const int i, const float f)
    {
      return fmt::format("free_func_data({}, {}, {})", *s, i, f);
    }

    struct widget_t {
      std::string member;

      std::string member_func(const int i, const float f)
      {
        member += "(called)";
        return fmt::format("widget_t::member_func({}, {}, {})", member, i, f);
      }

      std::string member_func_const(const int i, const float f) const
      {
        return fmt::format("widget_t::member_func_const({}, {}, {})", member, i, f);
      }
    };
  }

  TEST_CASE("delegate")
  {
    using dg_t = delegate_t<std::string(int, float)>;
    dg_t dg;
    dg = dg_t::make<&test::free_func>();
    CHECK(dg(42, 3.5) == "free_func(42, 3.5)");

    const std::string s{"foo"};
    dg = dg_t::make<&test::free_func_data>(&s);
    CHECK(dg(42, 3.5) == "free_func_data(foo, 42, 3.5)");

    test::widget_t widget{.member = "hi"};

    dg = dg_t::make<&test::widget_t::member_func_const>(&widget);
    CHECK(dg(42, 3.5) == "widget_t::member_func_const(hi, 42, 3.5)");

    dg = dg_t::make<&test::widget_t::member_func>(&widget);
    CHECK(dg(42, 3.5) == "widget_t::member_func(hi(called), 42, 3.5)");
    CHECK(dg(42, 3.5) == "widget_t::member_func(hi(called)(called), 42, 3.5)");
  }
}
