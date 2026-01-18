#include "interpreter.hpp"

#include "lox.hpp"
#include "test_suite.hpp"

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

    void prepare() { scopes.root().remove_all_definitions(); }

    void test_success(const string_view_t expr_str, const string_view_t expected)
    {
      prepare();
      INFO(expr_str);
      auto tp = SILVA_REQUIRE(tokenize(lexicon.swp, "test.lox", expr_str));
      INFO(pretty_string(*tp));
      auto pt = SILVA_REQUIRE(si->apply(tp, lexicon.swp->name_id_of("Lox")));
      INFO(pretty_string(pt->span()));
      SILVA_REQUIRE(resolve(pt->span()));
      auto scope = scopes.root();
      SILVA_REQUIRE(execute(pt->span(), scope));
      const auto result = print_buffer.content_str_fetch();
      CHECK(result == expected);
    };

    void test_runtime_error(const string_view_t expr_str)
    {
      prepare();
      INFO(expr_str);
      auto tp = SILVA_REQUIRE(tokenize(lexicon.swp, "test.lox", expr_str));
      INFO(pretty_string(*tp));
      auto pt = SILVA_REQUIRE(si->apply(tp, lexicon.swp->name_id_of("Lox")));
      INFO(pretty_string(pt->span()));
      SILVA_REQUIRE(resolve(pt->span()));
      auto scope        = scopes.root();
      const auto result = execute(pt->span(), scope);
      REQUIRE(!result.has_value());
    };
  };

  TEST_CASE("lox-evaluate", "[lox]")
  {
    syntax_ward_t sw;
    auto si = standard_seed_interpreter(sw.ptr());
    SILVA_REQUIRE(si->add_complete_file("lox.seed", lox::seed_str));
    test_interpreter_t lti{si.get(), sw.ptr()};

    const auto ts = test_suite();
    for (const auto& chapter: ts) {
      for (const auto& test_case: chapter.test_cases) {
        if (test_case.is_success_expected()) {
          lti.test_success(test_case.lox_code, std::get<string_view_t>(test_case.expected));
        }
        else {
          lti.test_runtime_error(test_case.lox_code);
        }
      }
    }
  }
}
