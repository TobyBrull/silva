#include "interpreter.hpp"

#include "lox.hpp"

#include "syntax/syntax.hpp"

#include <catch2/catch_all.hpp>

namespace silva::lox::test {
  struct test_interpreter_t : public interpreter_t {
    seed::interpreter_t* si = nullptr;

    byte_sink_memory_t print_buffer;

    test_interpreter_t(seed::interpreter_t* si, syntax_ward_ptr_t swp)
      : interpreter_t(std::move(swp), nullptr), si(si)
    {
      print_stream = &print_buffer;
    }

    void test(const string_view_t expr_str, const string_view_t expected)
    {
      INFO(expr_str);
      auto tp = SILVA_EXPECT_REQUIRE(tokenize(lexicon.swp, "test.lox", expr_str));
      INFO(pretty_string(*tp));
      auto pt = SILVA_EXPECT_REQUIRE(si->apply(tp, lexicon.swp->name_id_of("Lox")));
      INFO(pretty_string(pt->span()));
      SILVA_EXPECT_REQUIRE(resolve(pt->span()));
      auto scope = scopes.root();
      SILVA_EXPECT_REQUIRE(execute(pt->span(), scope));
      const auto result = print_buffer.content_str_fetch();
      CHECK(result == expected);
    };

    void test_runtime_error(const string_view_t expr_str)
    {
      INFO(expr_str);
      auto tp = SILVA_EXPECT_REQUIRE(tokenize(lexicon.swp, "test.lox", expr_str));
      INFO(pretty_string(*tp));
      auto pt = SILVA_EXPECT_REQUIRE(si->apply(tp, lexicon.swp->name_id_of("Lox")));
      INFO(pretty_string(pt->span()));
      SILVA_EXPECT_REQUIRE(resolve(pt->span()));
      auto scope        = scopes.root();
      const auto result = execute(pt->span(), scope);
      REQUIRE(!result.has_value());
    };
  };

  TEST_CASE("lox-evaluate", "[lox]")
  {
    syntax_ward_t sw;
    auto si = standard_seed_interpreter(sw.ptr());
    SILVA_EXPECT_REQUIRE(si->add_complete_file("lox.seed", lox::seed_str));
    test_interpreter_t lti{si.get(), sw.ptr()};

    lti.test("print ! 42 ;", "false\n");
    lti.test("print ! false ;", "true\n");
    lti.test("print ! true ;", "false\n");
    lti.test("print ! ! none ;", "false\n");
    lti.test(R"(print ! '' ;)", "false\n");
    lti.test("print - 42 ;", "-42\n");
    lti.test("print 1 + 2 * 3 - 4 / 2 ;", "5\n");
    lti.test("print '1' + '2' ;", "12\n");
    lti.test_runtime_error("print '1' + 2 ;");
    lti.test_runtime_error("print '1' * '2' ;");
    lti.test("print 1 < 3 ;", "true\n");
    lti.test("print 3 < 3 ;", "false\n");
    lti.test("print 1 <= 3 ;", "true\n");
    lti.test("print 3 <= 3 ;", "true\n");
    lti.test("print 4 <= 3 ;", "false\n");
    lti.test("print 3 > 1 ;", "true\n");
    lti.test("print 3 > 3 ;", "false\n");
    lti.test("print 3 >= 1 ;", "true\n");
    lti.test("print 3 >= 3 ;", "true\n");
    lti.test("print 3 >= 4 ;", "false\n");
    lti.test("print 3 == 3 ;", "true\n");
    lti.test("print 3 == '3' ;", "false\n");
    lti.test("print 3 != '3' ;", "true\n");
    lti.test("print true and true ;", "true\n");
    lti.test("print false and true ;", "false\n");
    lti.test("print true and false ;", "false\n");
    lti.test("print false and false ;", "false\n");
    lti.test("print true or true ;", "true\n");
    lti.test("print false or true ;", "true\n");
    lti.test("print true or false ;", "true\n");
    lti.test("print false or false ;", "false\n");
  }
}
