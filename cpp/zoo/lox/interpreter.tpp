#include "interpreter.hpp"

#include "lox.hpp"

#include "syntax/syntax.hpp"

#include <catch2/catch_all.hpp>

namespace silva::lox::test {
  struct test_interpreter_t : public interpreter_t {
    seed::interpreter_t* si = nullptr;

    test_interpreter_t(seed::interpreter_t* si, syntax_ward_ptr_t swp)
      : interpreter_t(std::move(swp)), si(si)
    {
    }

    expected_t<object_ref_t> eval(const string_view_t expr_str)
    {
      INFO(expr_str);
      auto tp = SILVA_EXPECT_REQUIRE(tokenize(swp, "test.lox", expr_str));
      INFO(to_string(*tp).as_string_view());
      auto pt = SILVA_EXPECT_REQUIRE(si->apply(tp, swp->name_id_of("Lox", "Expr")));
      INFO(to_string(pt->span()).as_string_view());
      SILVA_EXPECT_REQUIRE(resolve(pt->span()));
      auto retval = evaluate(pt->span(), scopes.root());
      return retval;
    };
  };

  TEST_CASE("lox-evaluate", "[lox]")
  {
    syntax_ward_t sw;
    auto si = standard_seed_interpreter(sw.ptr());
    SILVA_EXPECT_REQUIRE(si->add_complete_file("lox.seed", lox::seed_str));
    test_interpreter_t lti{si.get(), sw.ptr()};

    CHECK(lti.eval("!42").value()->is_truthy() == false);
    CHECK(lti.eval("!false").value()->is_truthy());
    CHECK(lti.eval("!true").value()->is_truthy() == false);
    CHECK(lti.eval("! ! none").value()->is_truthy() == false);
    CHECK(lti.eval(R"(!'')").value()->is_truthy() == false);
    CHECK(lti.eval("-42").value()->as_double() == -42.0);
    CHECK(*lti.eval("1 + 2 * 3 - 4 / 2").value()->as_double() == 5.0);
    CHECK(*lti.eval("'1' + '2'").value()->as_string() == "12");
    CHECK(lti.eval("'1' + 2").has_value() == false);
    CHECK(lti.eval("'1' * '2'").has_value() == false);
    CHECK(lti.eval("1 < 3").value()->is_truthy());
    CHECK(lti.eval("3 < 3").value()->is_truthy() == false);
    CHECK(lti.eval("1 <= 3").value()->is_truthy());
    CHECK(lti.eval("3 <= 3").value()->is_truthy());
    CHECK(lti.eval("4 <= 3").value()->is_truthy() == false);
    CHECK(lti.eval("3 > 1").value()->is_truthy());
    CHECK(lti.eval("3 > 3").value()->is_truthy() == false);
    CHECK(lti.eval("3 >= 1").value()->is_truthy());
    CHECK(lti.eval("3 >= 3").value()->is_truthy());
    CHECK(lti.eval("3 >= 4").value()->is_truthy() == false);
    CHECK(lti.eval("3 == 3").value()->is_truthy());
    CHECK(lti.eval("3 == '3'").value()->is_truthy() == false);
    CHECK(lti.eval("3 != '3'").value()->is_truthy());
    CHECK(lti.eval("true and true").value()->is_truthy());
    CHECK(lti.eval("false and true").value()->is_truthy() == false);
    CHECK(lti.eval("true and false").value()->is_truthy() == false);
    CHECK(lti.eval("false and false").value()->is_truthy() == false);
    CHECK(lti.eval("true or true").value()->is_truthy());
    CHECK(lti.eval("false or true").value()->is_truthy());
    CHECK(lti.eval("true or false").value()->is_truthy());
    CHECK(lti.eval("false or false").value()->is_truthy() == false);
  }
}
