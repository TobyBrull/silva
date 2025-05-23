#include "interpreter.hpp"

#include "lox.hpp"

#include "syntax/syntax.hpp"

#include <catch2/catch_all.hpp>

namespace silva::lox::test {
  struct test_interpreter_t : public interpreter_t {
    seed::interpreter_t* si = nullptr;

    value_t eval(const string_view_t expr_str)
    {
      INFO(expr_str);
      auto tp = SILVA_EXPECT_REQUIRE(tokenize(swp, "test.lox", expr_str));
      INFO(to_string(*tp).as_string_view());
      auto pt = SILVA_EXPECT_REQUIRE(si->apply(tp, swp->name_id_of("Lox", "Expr")));
      INFO(to_string(pt->span()).as_string_view());
      auto res = SILVA_EXPECT_REQUIRE(evaluate(pt->span()));
      return res;
    };
  };

  TEST_CASE("lox-evaluate", "[lox]")
  {
    syntax_ward_t sw;
    auto si = standard_seed_engine(sw.ptr());
    SILVA_EXPECT_REQUIRE(si->add_complete_file("lox.seed", lox::seed_str));
    test_interpreter_t lti{{sw.ptr()}, si.get()};

    CHECK(lti.eval("!42") == value_t{false});
    CHECK(lti.eval("!false") == value_t{true});
    CHECK(lti.eval("!true") == value_t{false});
    CHECK(lti.eval("! ! none") == value_t{false});
    CHECK(lti.eval(R"(!'')") == value_t{false});
    CHECK(lti.eval("-42") == value_t{-42.0});
  }
}
