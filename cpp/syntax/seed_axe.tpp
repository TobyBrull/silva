#include "seed_axe.hpp"

#include "syntax.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;
using namespace silva::seed::impl;
using enum silva::seed::impl::assoc_t;

namespace silva::seed::test {
  using enum fragment_category_t;

  void test_axe(seed::interpreter_t& si,
                const axe_t& pa,
                const string_view_t text,
                const optional_t<string_view_t> expected_str)
  {
    INFO(text);
    auto fp       = SILVA_REQUIRE(fragmentize(si.sfp, "", string_t{text}));
    auto maybe_pt = si.apply(fp, si.sfp->name_id_of("Test"));
    optional_t<string_t> result_str;
    if (maybe_pt.has_value()) {
      auto result_pt = *std::move(maybe_pt);
      result_str     = SILVA_REQUIRE(result_pt->span().to_string());
      UNSCOPED_INFO(result_str.value());
    }
    else {
      UNSCOPED_INFO(pretty_string(maybe_pt.error()));
    }
    REQUIRE(maybe_pt.has_value() == expected_str.has_value());
    if (!expected_str.has_value()) {
      return;
    }
    CHECK(result_str.value() == expected_str.value().substr(1));
  }

  TEST_CASE("seed-axe-basic", "[seed-axe]")
  {
    syntax_farm_t sf;
    const unique_ptr_t<seed::interpreter_t> se = standard_seed_interpreter(sf.ptr());
    const string_view_t test_axe_str           = R"'(
language Test:
  ⊙ = axe Atom oper
    Dot   = rtl   infix '.'
    Sub   = ltr   postfix_nest '[' ']'
    Dol   = ltr   postfix '$'
    Exc   = ltr   postfix '!'
    Til   = rtl   prefix '~'
    Prf   = rtl   prefix '+' '-'
    Mul   = ltr   infix '*' '/'
    Add   = ltr   infix '+' '-'
    Ter   = rtl   ternary '?' ':'
    Eqa   = rtl   infix '='
  oper = operator.single | parenthesis
  Atom = identifier | number | '(' Test ')'
  skip = skip.freeForm
)'";
    SILVA_REQUIRE(se->add_seed_text("test.seed", string_t{test_axe_str}));
    const auto& sa = se->axes.at(sf.name_id_of("Test"));
    CHECK(!sa.concat_result.has_value());
    CHECK(sa.results.size() == 13);
    {
      const axe_result_t rr = sa.results.at(sf.token_id("="));
      const axe_result_t expected{
          .prefix = {none},
          .regular =
              result_oper_t<oper_regular_t>{
                  .oper       = infix_t{sf.token_id("=")},
                  .name       = sf.name_id_of("Test", "Eqa", "="),
                  .precedence = precedence_t{.level_index = 1, .assoc = RIGHT_TO_LEFT},
                  .pts        = rr.regular->pts,
              },
          .is_right_bracket = false,
      };
      CHECK(rr == expected);
    }
    {
      const axe_result_t rr = sa.results.at(sf.token_id("?"));
      const axe_result_t expected{
          .prefix = {none},
          .regular =
              result_oper_t<oper_regular_t>{
                  .oper       = ternary_t{sf.token_id("?"), sf.token_id(":")},
                  .name       = sf.name_id_of("Test", "Ter", "?"),
                  .precedence = precedence_t{.level_index = 2, .assoc = RIGHT_TO_LEFT},
                  .pts        = rr.regular->pts,
              },
          .is_right_bracket = false,
      };
      CHECK(rr == expected);
    }
    {
      const axe_result_t rr = sa.results.at(sf.token_id(":"));
      const axe_result_t expected{
          .prefix           = {none},
          .regular          = {none},
          .is_right_bracket = true,
      };
      CHECK(rr == expected);
    }
    {
      const axe_result_t rr = sa.results.at(sf.token_id("+"));
      const axe_result_t expected{
          .prefix =
              result_oper_t<oper_prefix_t>{
                  .oper       = prefix_t{sf.token_id("+")},
                  .name       = sf.name_id_of("Test", "Prf", "+"),
                  .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                  .pts        = rr.prefix->pts,
              },
          .regular =
              result_oper_t<oper_regular_t>{
                  .oper       = infix_t{sf.token_id("+")},
                  .name       = sf.name_id_of("Test", "Add", "+"),
                  .precedence = precedence_t{.level_index = 3, .assoc = LEFT_TO_RIGHT},
                  .pts        = rr.regular->pts,
              },
          .is_right_bracket = false,
      };
      CHECK(rr == expected);
    }
    {
      const axe_result_t rr = sa.results.at(sf.token_id("-"));
      const axe_result_t expected{
          .prefix =
              result_oper_t<oper_prefix_t>{
                  .oper       = prefix_t{sf.token_id("-")},
                  .name       = sf.name_id_of("Test", "Prf", "-"),
                  .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                  .pts        = rr.prefix->pts,
              },
          .regular =
              result_oper_t<oper_regular_t>{
                  .oper       = infix_t{sf.token_id("-")},
                  .name       = sf.name_id_of("Test", "Add", "-"),
                  .precedence = precedence_t{.level_index = 3, .assoc = LEFT_TO_RIGHT},
                  .pts        = rr.regular->pts,
              },
          .is_right_bracket = false,
      };
      CHECK(rr == expected);
    }

    test::test_axe(*se, sa, "1\n", R"(
[0].Test                                          ｢1<NEWLINE>｣
  [0].Test.Atom                                   ｢1<NEWLINE>｣
    [0].number                                    ｢1｣
      [0].number.integer                          ｢1｣
        [0].number.integer.decimal                ｢1｣
          [0].number.plusMinus                    ｢｣
          [1].number.unsigned.integer.decimal     ｢1｣
)");
    test::test_axe(*se, sa, "1 + 2\n", R"(
[0].Test                                          ｢1 + 2<NEWLINE>｣
  [0].Test.Add.+                                  ｢1 + 2<NEWLINE>｣
    [0].Test.Atom                                 ｢1 ｣
      [0].number                                  ｢1｣
        [0].number.integer                        ｢1｣
          [0].number.integer.decimal              ｢1｣
            [0].number.plusMinus                  ｢｣
            [1].number.unsigned.integer.decimal   ｢1｣
    [1].Test.oper                                 ｢+｣
    [2].Test.Atom                                 ｢2<NEWLINE>｣
      [0].number                                  ｢2｣
        [0].number.integer                        ｢2｣
          [0].number.integer.decimal              ｢2｣
            [0].number.plusMinus                  ｢｣
            [1].number.unsigned.integer.decimal   ｢2｣
)");
    test::test_axe(*se, sa, "1 - 2\n", R"(
[0].Test                                          ｢1 - 2<NEWLINE>｣
  [0].Test.Add.-                                  ｢1 - 2<NEWLINE>｣
    [0].Test.Atom                                 ｢1 ｣
      [0].number                                  ｢1｣
        [0].number.integer                        ｢1｣
          [0].number.integer.decimal              ｢1｣
            [0].number.plusMinus                  ｢｣
            [1].number.unsigned.integer.decimal   ｢1｣
    [1].Test.oper                                 ｢-｣
    [2].Test.Atom                                 ｢2<NEWLINE>｣
      [0].number                                  ｢2｣
        [0].number.integer                        ｢2｣
          [0].number.integer.decimal              ｢2｣
            [0].number.plusMinus                  ｢｣
            [1].number.unsigned.integer.decimal   ｢2｣
)");
    test::test_axe(*se, sa, "1 + 2 * 3 + 4\n", R"(
[0].Test                                          ｢1 + 2 ...  + 4<NEWLINE>｣
  [0].Test.Add.+                                  ｢1 + 2 ...  + 4<NEWLINE>｣
    [0].Test.Add.+                                ｢1 + 2 * 3 ｣
      [0].Test.Atom                               ｢1 ｣
        [0].number                                ｢1｣
          [0].number.integer                      ｢1｣
            [0].number.integer.decimal            ｢1｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢1｣
      [1].Test.oper                               ｢+｣
      [2].Test.Mul.*                              ｢2 * 3 ｣
        [0].Test.Atom                             ｢2 ｣
          [0].number                              ｢2｣
            [0].number.integer                    ｢2｣
              [0].number.integer.decimal          ｢2｣
                [0].number.plusMinus              ｢｣
                [1].number.unsigned.integer.decimal ｢2｣
        [1].Test.oper                             ｢*｣
        [2].Test.Atom                             ｢3 ｣
          [0].number                              ｢3｣
            [0].number.integer                    ｢3｣
              [0].number.integer.decimal          ｢3｣
                [0].number.plusMinus              ｢｣
                [1].number.unsigned.integer.decimal ｢3｣
    [1].Test.oper                                 ｢+｣
    [2].Test.Atom                                 ｢4<NEWLINE>｣
      [0].number                                  ｢4｣
        [0].number.integer                        ｢4｣
          [0].number.integer.decimal              ｢4｣
            [0].number.plusMinus                  ｢｣
            [1].number.unsigned.integer.decimal   ｢4｣
)");
    test::test_axe(*se, sa, "1 - 2 + f . g . h * 3 / 4\n", R"(
[0].Test                                          ｢1 - 2 ...  / 4<NEWLINE>｣
  [0].Test.Add.+                                  ｢1 - 2 ...  / 4<NEWLINE>｣
    [0].Test.Add.-                                ｢1 - 2 ｣
      [0].Test.Atom                               ｢1 ｣
        [0].number                                ｢1｣
          [0].number.integer                      ｢1｣
            [0].number.integer.decimal            ｢1｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢1｣
      [1].Test.oper                               ｢-｣
      [2].Test.Atom                               ｢2 ｣
        [0].number                                ｢2｣
          [0].number.integer                      ｢2｣
            [0].number.integer.decimal            ｢2｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢2｣
    [1].Test.oper                                 ｢+｣
    [2].Test.Mul./                                ｢f . g ...  / 4<NEWLINE>｣
      [0].Test.Mul.*                              ｢f . g ...  * 3 ｣
        [0].Test.Dot..                            ｢f . g . h ｣
          [0].Test.Atom                           ｢f ｣
            [0].identifier                        ｢f｣
          [1].Test.oper                           ｢.｣
          [2].Test.Dot..                          ｢g . h ｣
            [0].Test.Atom                         ｢g ｣
              [0].identifier                      ｢g｣
            [1].Test.oper                         ｢.｣
            [2].Test.Atom                         ｢h ｣
              [0].identifier                      ｢h｣
        [1].Test.oper                             ｢*｣
        [2].Test.Atom                             ｢3 ｣
          [0].number                              ｢3｣
            [0].number.integer                    ｢3｣
              [0].number.integer.decimal          ｢3｣
                [0].number.plusMinus              ｢｣
                [1].number.unsigned.integer.decimal ｢3｣
      [1].Test.oper                               ｢/｣
      [2].Test.Atom                               ｢4<NEWLINE>｣
        [0].number                                ｢4｣
          [0].number.integer                      ｢4｣
            [0].number.integer.decimal            ｢4｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢4｣
)");
    test::test_axe(*se, sa, "2 ! + 3\n", R"(
[0].Test                                          ｢2 ! + 3<NEWLINE>｣
  [0].Test.Add.+                                  ｢2 ! + 3<NEWLINE>｣
    [0].Test.Exc.!                                ｢2 ! ｣
      [0].Test.Atom                               ｢2 ｣
        [0].number                                ｢2｣
          [0].number.integer                      ｢2｣
            [0].number.integer.decimal            ｢2｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢2｣
      [1].Test.oper                               ｢!｣
    [1].Test.oper                                 ｢+｣
    [2].Test.Atom                                 ｢3<NEWLINE>｣
      [0].number                                  ｢3｣
        [0].number.integer                        ｢3｣
          [0].number.integer.decimal              ｢3｣
            [0].number.plusMinus                  ｢｣
            [1].number.unsigned.integer.decimal   ｢3｣
)");
    test::test_axe(*se, sa, " - + 1\n", R"(
[0].Test                                          ｢- + 1<NEWLINE><DEDENT>｣
  [0].Test.Prf.-                                  ｢- + 1<NEWLINE><DEDENT>｣
    [0].Test.oper                                 ｢-｣
    [1].Test.Prf.+                                ｢+ 1<NEWLINE><DEDENT>｣
      [0].Test.oper                               ｢+｣
      [1].Test.Atom                               ｢1<NEWLINE><DEDENT>｣
        [0].number                                ｢1｣
          [0].number.integer                      ｢1｣
            [0].number.integer.decimal            ｢1｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢1｣
)");
    test::test_axe(*se, sa, "a + - + 1\n", R"(
[0].Test                                          ｢a + - + 1<NEWLINE>｣
  [0].Test.Add.+                                  ｢a + - + 1<NEWLINE>｣
    [0].Test.Atom                                 ｢a ｣
      [0].identifier                              ｢a｣
    [1].Test.oper                                 ｢+｣
    [2].Test.Prf.-                                ｢- + 1<NEWLINE>｣
      [0].Test.oper                               ｢-｣
      [1].Test.Prf.+                              ｢+ 1<NEWLINE>｣
        [0].Test.oper                             ｢+｣
        [1].Test.Atom                             ｢1<NEWLINE>｣
          [0].number                              ｢1｣
            [0].number.integer                    ｢1｣
              [0].number.integer.decimal          ｢1｣
                [0].number.plusMinus              ｢｣
                [1].number.unsigned.integer.decimal ｢1｣
)");
    test::test_axe(*se, sa, "- - 1 * 2\n", R"(
[0].Test                                          ｢- - 1 * 2<NEWLINE>｣
  [0].Test.Mul.*                                  ｢- - 1 * 2<NEWLINE>｣
    [0].Test.Prf.-                                ｢- - 1 ｣
      [0].Test.oper                               ｢-｣
      [1].Test.Prf.-                              ｢- 1 ｣
        [0].Test.oper                             ｢-｣
        [1].Test.Atom                             ｢1 ｣
          [0].number                              ｢1｣
            [0].number.integer                    ｢1｣
              [0].number.integer.decimal          ｢1｣
                [0].number.plusMinus              ｢｣
                [1].number.unsigned.integer.decimal ｢1｣
    [1].Test.oper                                 ｢*｣
    [2].Test.Atom                                 ｢2<NEWLINE>｣
      [0].number                                  ｢2｣
        [0].number.integer                        ｢2｣
          [0].number.integer.decimal              ｢2｣
            [0].number.plusMinus                  ｢｣
            [1].number.unsigned.integer.decimal   ｢2｣
)");
    test::test_axe(*se, sa, "- - 1 . 2\n", R"(
[0].Test                                          ｢- - 1 . 2<NEWLINE>｣
  [0].Test.Prf.-                                  ｢- - 1 . 2<NEWLINE>｣
    [0].Test.oper                                 ｢-｣
    [1].Test.Prf.-                                ｢- 1 . 2<NEWLINE>｣
      [0].Test.oper                               ｢-｣
      [1].Test.Dot..                              ｢1 . 2<NEWLINE>｣
        [0].Test.Atom                             ｢1 ｣
          [0].number                              ｢1｣
            [0].number.integer                    ｢1｣
              [0].number.integer.decimal          ｢1｣
                [0].number.plusMinus              ｢｣
                [1].number.unsigned.integer.decimal ｢1｣
        [1].Test.oper                             ｢.｣
        [2].Test.Atom                             ｢2<NEWLINE>｣
          [0].number                              ｢2｣
            [0].number.integer                    ｢2｣
              [0].number.integer.decimal          ｢2｣
                [0].number.plusMinus              ｢｣
                [1].number.unsigned.integer.decimal ｢2｣
)");
    test::test_axe(*se, sa, "1 . 2 !\n", R"(
[0].Test                                          ｢1 . 2 !<NEWLINE>｣
  [0].Test.Exc.!                                  ｢1 . 2 !<NEWLINE>｣
    [0].Test.Dot..                                ｢1 . 2 ｣
      [0].Test.Atom                               ｢1 ｣
        [0].number                                ｢1｣
          [0].number.integer                      ｢1｣
            [0].number.integer.decimal            ｢1｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢1｣
      [1].Test.oper                               ｢.｣
      [2].Test.Atom                               ｢2 ｣
        [0].number                                ｢2｣
          [0].number.integer                      ｢2｣
            [0].number.integer.decimal            ｢2｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢2｣
    [1].Test.oper                                 ｢!｣
)");
    test::test_axe(*se, sa, "1 + 2 !\n", R"(
[0].Test                                          ｢1 + 2 !<NEWLINE>｣
  [0].Test.Add.+                                  ｢1 + 2 !<NEWLINE>｣
    [0].Test.Atom                                 ｢1 ｣
      [0].number                                  ｢1｣
        [0].number.integer                        ｢1｣
          [0].number.integer.decimal              ｢1｣
            [0].number.plusMinus                  ｢｣
            [1].number.unsigned.integer.decimal   ｢1｣
    [1].Test.oper                                 ｢+｣
    [2].Test.Exc.!                                ｢2 !<NEWLINE>｣
      [0].Test.Atom                               ｢2 ｣
        [0].number                                ｢2｣
          [0].number.integer                      ｢2｣
            [0].number.integer.decimal            ｢2｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢2｣
      [1].Test.oper                               ｢!｣
)");
    test::test_axe(*se, sa, "2 ! . 3\n", {none});
    test::test_axe(*se, sa, "2 . - 3\n", {none});
    test::test_axe(*se, sa, "2 $ !\n", R"(
[0].Test                                          ｢2 $ !<NEWLINE>｣
  [0].Test.Exc.!                                  ｢2 $ !<NEWLINE>｣
    [0].Test.Dol.$                                ｢2 $ ｣
      [0].Test.Atom                               ｢2 ｣
        [0].number                                ｢2｣
          [0].number.integer                      ｢2｣
            [0].number.integer.decimal            ｢2｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢2｣
      [1].Test.oper                               ｢$｣
    [1].Test.oper                                 ｢!｣
)");
    test::test_axe(*se, sa, "2 ! $\n", {none});
    test::test_axe(*se, sa, "+ ~ 2\n", R"(
[0].Test                                          ｢+ ~ 2<NEWLINE>｣
  [0].Test.Prf.+                                  ｢+ ~ 2<NEWLINE>｣
    [0].Test.oper                                 ｢+｣
    [1].Test.Til.~                                ｢~ 2<NEWLINE>｣
      [0].Test.oper                               ｢~｣
      [1].Test.Atom                               ｢2<NEWLINE>｣
        [0].number                                ｢2｣
          [0].number.integer                      ｢2｣
            [0].number.integer.decimal            ｢2｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢2｣
)");
    test::test_axe(*se, sa, "~ + 2\n", {none});
    test::test_axe(*se, sa, "( ( 0 ) )\n", R"(
[0].Test                                          ｢( ( 0 ) )<NEWLINE>｣
  [0].Test.Atom                                   ｢( ( 0 ) )<NEWLINE>｣
    [0].Test                                      ｢( 0 ) ｣
      [0].Test.Atom                               ｢( 0 ) ｣
        [0].Test                                  ｢0 ｣
          [0].Test.Atom                           ｢0 ｣
            [0].number                            ｢0｣
              [0].number.integer                  ｢0｣
                [0].number.integer.decimal        ｢0｣
                  [0].number.plusMinus            ｢｣
                  [1].number.unsigned.integer.decimal ｢0｣
)");
    test::test_axe(*se, sa, "1 * ( 2 + 3 ) * 4\n", R"(
[0].Test                                          ｢1 * ( ...  * 4<NEWLINE>｣
  [0].Test.Mul.*                                  ｢1 * ( ...  * 4<NEWLINE>｣
    [0].Test.Mul.*                                ｢1 * ( ...  3 ) ｣
      [0].Test.Atom                               ｢1 ｣
        [0].number                                ｢1｣
          [0].number.integer                      ｢1｣
            [0].number.integer.decimal            ｢1｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢1｣
      [1].Test.oper                               ｢*｣
      [2].Test.Atom                               ｢( 2 + 3 ) ｣
        [0].Test                                  ｢2 + 3 ｣
          [0].Test.Add.+                          ｢2 + 3 ｣
            [0].Test.Atom                         ｢2 ｣
              [0].number                          ｢2｣
                [0].number.integer                ｢2｣
                  [0].number.integer.decimal      ｢2｣
                    [0].number.plusMinus          ｢｣
                    [1].number.unsigned.integer.decimal ｢2｣
            [1].Test.oper                         ｢+｣
            [2].Test.Atom                         ｢3 ｣
              [0].number                          ｢3｣
                [0].number.integer                ｢3｣
                  [0].number.integer.decimal      ｢3｣
                    [0].number.plusMinus          ｢｣
                    [1].number.unsigned.integer.decimal ｢3｣
    [1].Test.oper                                 ｢*｣
    [2].Test.Atom                                 ｢4<NEWLINE>｣
      [0].number                                  ｢4｣
        [0].number.integer                        ｢4｣
          [0].number.integer.decimal              ｢4｣
            [0].number.plusMinus                  ｢｣
            [1].number.unsigned.integer.decimal   ｢4｣
)");
    test::test_axe(*se, sa, "1 * ( 2 + 3 ) * 4\n", R"(
[0].Test                                          ｢1 * ( ...  * 4<NEWLINE>｣
  [0].Test.Mul.*                                  ｢1 * ( ...  * 4<NEWLINE>｣
    [0].Test.Mul.*                                ｢1 * ( ...  3 ) ｣
      [0].Test.Atom                               ｢1 ｣
        [0].number                                ｢1｣
          [0].number.integer                      ｢1｣
            [0].number.integer.decimal            ｢1｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢1｣
      [1].Test.oper                               ｢*｣
      [2].Test.Atom                               ｢( 2 + 3 ) ｣
        [0].Test                                  ｢2 + 3 ｣
          [0].Test.Add.+                          ｢2 + 3 ｣
            [0].Test.Atom                         ｢2 ｣
              [0].number                          ｢2｣
                [0].number.integer                ｢2｣
                  [0].number.integer.decimal      ｢2｣
                    [0].number.plusMinus          ｢｣
                    [1].number.unsigned.integer.decimal ｢2｣
            [1].Test.oper                         ｢+｣
            [2].Test.Atom                         ｢3 ｣
              [0].number                          ｢3｣
                [0].number.integer                ｢3｣
                  [0].number.integer.decimal      ｢3｣
                    [0].number.plusMinus          ｢｣
                    [1].number.unsigned.integer.decimal ｢3｣
    [1].Test.oper                                 ｢*｣
    [2].Test.Atom                                 ｢4<NEWLINE>｣
      [0].number                                  ｢4｣
        [0].number.integer                        ｢4｣
          [0].number.integer.decimal              ｢4｣
            [0].number.plusMinus                  ｢｣
            [1].number.unsigned.integer.decimal   ｢4｣
)");
    test::test_axe(*se, sa, "a [ 0 ]\n", R"(
[0].Test                                          ｢a [ 0 ]<NEWLINE>｣
  [0].Test.Sub.[                                  ｢a [ 0 ]<NEWLINE>｣
    [0].Test.Atom                                 ｢a ｣
      [0].identifier                              ｢a｣
    [1].Test.oper                                 ｢[｣
      [0].parenthesis                             ｢[｣
    [2].Test                                      ｢0 ｣
      [0].Test.Atom                               ｢0 ｣
        [0].number                                ｢0｣
          [0].number.integer                      ｢0｣
            [0].number.integer.decimal            ｢0｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢0｣
    [3].Test.oper                                 ｢]｣
      [0].parenthesis                             ｢]｣
)");
    test::test_axe(*se, sa, "a [ 0 ] [ 1 ]\n", R"(
[0].Test                                          ｢a [ 0 ...  1 ]<NEWLINE>｣
  [0].Test.Sub.[                                  ｢a [ 0 ...  1 ]<NEWLINE>｣
    [0].Test.Sub.[                                ｢a [ 0 ] ｣
      [0].Test.Atom                               ｢a ｣
        [0].identifier                            ｢a｣
      [1].Test.oper                               ｢[｣
        [0].parenthesis                           ｢[｣
      [2].Test                                    ｢0 ｣
        [0].Test.Atom                             ｢0 ｣
          [0].number                              ｢0｣
            [0].number.integer                    ｢0｣
              [0].number.integer.decimal          ｢0｣
                [0].number.plusMinus              ｢｣
                [1].number.unsigned.integer.decimal ｢0｣
      [3].Test.oper                               ｢]｣
        [0].parenthesis                           ｢]｣
    [1].Test.oper                                 ｢[｣
      [0].parenthesis                             ｢[｣
    [2].Test                                      ｢1 ｣
      [0].Test.Atom                               ｢1 ｣
        [0].number                                ｢1｣
          [0].number.integer                      ｢1｣
            [0].number.integer.decimal            ｢1｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢1｣
    [3].Test.oper                                 ｢]｣
      [0].parenthesis                             ｢]｣
)");
    test::test_axe(*se, sa, "a [ 0 ] . b [ 1 ]\n", {none});
    test::test_axe(*se, sa, "a [ 0 ] + b [ 1 ]\n", R"(
[0].Test                                          ｢a [ 0 ...  1 ]<NEWLINE>｣
  [0].Test.Add.+                                  ｢a [ 0 ...  1 ]<NEWLINE>｣
    [0].Test.Sub.[                                ｢a [ 0 ] ｣
      [0].Test.Atom                               ｢a ｣
        [0].identifier                            ｢a｣
      [1].Test.oper                               ｢[｣
        [0].parenthesis                           ｢[｣
      [2].Test                                    ｢0 ｣
        [0].Test.Atom                             ｢0 ｣
          [0].number                              ｢0｣
            [0].number.integer                    ｢0｣
              [0].number.integer.decimal          ｢0｣
                [0].number.plusMinus              ｢｣
                [1].number.unsigned.integer.decimal ｢0｣
      [3].Test.oper                               ｢]｣
        [0].parenthesis                           ｢]｣
    [1].Test.oper                                 ｢+｣
    [2].Test.Sub.[                                ｢b [ 1 ]<NEWLINE>｣
      [0].Test.Atom                               ｢b ｣
        [0].identifier                            ｢b｣
      [1].Test.oper                               ｢[｣
        [0].parenthesis                           ｢[｣
      [2].Test                                    ｢1 ｣
        [0].Test.Atom                             ｢1 ｣
          [0].number                              ｢1｣
            [0].number.integer                    ｢1｣
              [0].number.integer.decimal          ｢1｣
                [0].number.plusMinus              ｢｣
                [1].number.unsigned.integer.decimal ｢1｣
      [3].Test.oper                               ｢]｣
        [0].parenthesis                           ｢]｣
)");
    test::test_axe(*se, sa, "a ? b : c\n", R"(
[0].Test                                          ｢a ? b : c<NEWLINE>｣
  [0].Test.Ter.?                                  ｢a ? b : c<NEWLINE>｣
    [0].Test.Atom                                 ｢a ｣
      [0].identifier                              ｢a｣
    [1].Test.oper                                 ｢?｣
    [2].Test                                      ｢b ｣
      [0].Test.Atom                               ｢b ｣
        [0].identifier                            ｢b｣
    [3].Test.oper                                 ｢:｣
    [4].Test.Atom                                 ｢c<NEWLINE>｣
      [0].identifier                              ｢c｣
)");
    test::test_axe(*se, sa, "a ? b : c ? d : e\n", R"(
[0].Test                                          ｢a ? b ...  : e<NEWLINE>｣
  [0].Test.Ter.?                                  ｢a ? b ...  : e<NEWLINE>｣
    [0].Test.Atom                                 ｢a ｣
      [0].identifier                              ｢a｣
    [1].Test.oper                                 ｢?｣
    [2].Test                                      ｢b ｣
      [0].Test.Atom                               ｢b ｣
        [0].identifier                            ｢b｣
    [3].Test.oper                                 ｢:｣
    [4].Test.Ter.?                                ｢c ? d : e<NEWLINE>｣
      [0].Test.Atom                               ｢c ｣
        [0].identifier                            ｢c｣
      [1].Test.oper                               ｢?｣
      [2].Test                                    ｢d ｣
        [0].Test.Atom                             ｢d ｣
          [0].identifier                          ｢d｣
      [3].Test.oper                               ｢:｣
      [4].Test.Atom                               ｢e<NEWLINE>｣
        [0].identifier                            ｢e｣
)");
    test::test_axe(*se, sa, "a ? b ? c : d : e\n", R"(
[0].Test                                          ｢a ? b ...  : e<NEWLINE>｣
  [0].Test.Ter.?                                  ｢a ? b ...  : e<NEWLINE>｣
    [0].Test.Atom                                 ｢a ｣
      [0].identifier                              ｢a｣
    [1].Test.oper                                 ｢?｣
    [2].Test                                      ｢b ? c : d ｣
      [0].Test.Ter.?                              ｢b ? c : d ｣
        [0].Test.Atom                             ｢b ｣
          [0].identifier                          ｢b｣
        [1].Test.oper                             ｢?｣
        [2].Test                                  ｢c ｣
          [0].Test.Atom                           ｢c ｣
            [0].identifier                        ｢c｣
        [3].Test.oper                             ｢:｣
        [4].Test.Atom                             ｢d ｣
          [0].identifier                          ｢d｣
    [3].Test.oper                                 ｢:｣
    [4].Test.Atom                                 ｢e<NEWLINE>｣
      [0].identifier                              ｢e｣
)");
    test::test_axe(*se, sa, "a = b ? c = d : e = f\n", R"(
[0].Test                                          ｢a = b ...  = f<NEWLINE>｣
  [0].Test.Eqa.=                                  ｢a = b ...  = f<NEWLINE>｣
    [0].Test.Atom                                 ｢a ｣
      [0].identifier                              ｢a｣
    [1].Test.oper                                 ｢=｣
    [2].Test.Eqa.=                                ｢b ? c ...  = f<NEWLINE>｣
      [0].Test.Ter.?                              ｢b ? c ...  : e ｣
        [0].Test.Atom                             ｢b ｣
          [0].identifier                          ｢b｣
        [1].Test.oper                             ｢?｣
        [2].Test                                  ｢c = d ｣
          [0].Test.Eqa.=                          ｢c = d ｣
            [0].Test.Atom                         ｢c ｣
              [0].identifier                      ｢c｣
            [1].Test.oper                         ｢=｣
            [2].Test.Atom                         ｢d ｣
              [0].identifier                      ｢d｣
        [3].Test.oper                             ｢:｣
        [4].Test.Atom                             ｢e ｣
          [0].identifier                          ｢e｣
      [1].Test.oper                               ｢=｣
      [2].Test.Atom                               ｢f<NEWLINE>｣
        [0].identifier                            ｢f｣
)");
    test::test_axe(*se, sa, "a + b ? c + d : e + f\n", R"(
[0].Test                                          ｢a + b ...  + f<NEWLINE>｣
  [0].Test.Ter.?                                  ｢a + b ...  + f<NEWLINE>｣
    [0].Test.Add.+                                ｢a + b ｣
      [0].Test.Atom                               ｢a ｣
        [0].identifier                            ｢a｣
      [1].Test.oper                               ｢+｣
      [2].Test.Atom                               ｢b ｣
        [0].identifier                            ｢b｣
    [1].Test.oper                                 ｢?｣
    [2].Test                                      ｢c + d ｣
      [0].Test.Add.+                              ｢c + d ｣
        [0].Test.Atom                             ｢c ｣
          [0].identifier                          ｢c｣
        [1].Test.oper                             ｢+｣
        [2].Test.Atom                             ｢d ｣
          [0].identifier                          ｢d｣
    [3].Test.oper                                 ｢:｣
    [4].Test.Add.+                                ｢e + f<NEWLINE>｣
      [0].Test.Atom                               ｢e ｣
        [0].identifier                            ｢e｣
      [1].Test.oper                               ｢+｣
      [2].Test.Atom                               ｢f<NEWLINE>｣
        [0].identifier                            ｢f｣
)");
  }

  TEST_CASE("seed-axe-advanced", "[seed-axe]")
  {
    syntax_farm_t sf;
    const unique_ptr_t<seed::interpreter_t> se = standard_seed_interpreter(sf.ptr());
    const string_view_t test_axe_str           = R"'(
language Test:
  ⊙ = axe Atom oper
    PrfHi   = rtl   prefix_nest '(' ')'
    Cat     = ltr   infix concat
    PrfLo   = rtl   prefix_nest '{' '}' prefix_nest -> Args '<:' ':>'
    Mul     = ltr   infix '*'
    Add     = ltr   infix_flat '+' infix '-'
    Assign  = rtl   infix_flat '=' infix '%'
  oper = '<:' | ':>' | operator.single | parenthesis
  Atom = identifier | number operator.single | '(' Test ')' | '<<' Test.PrfLo '>>'
  Args = string ( ',' string ) * | ε
  skip = skip.freeForm
)'";
    SILVA_REQUIRE(se->add_seed_text("test.seed", string_t{test_axe_str}));
    const auto& sa = se->axes.at(sf.name_id_of("Test"));
    CHECK(sa.concat_result.has_value());
    CHECK(sa.results.size() == 11);

    test::test_axe(*se, sa, "a\n", R"(
[0].Test                                          ｢a<NEWLINE>｣
  [0].Test.Atom                                   ｢a<NEWLINE>｣
    [0].identifier                                ｢a｣
)");
    test::test_axe(*se, sa, "a y z\n", R"(
[0].Test                                          ｢a y z<NEWLINE>｣
  [0].Test.Cat.concat                             ｢a y z<NEWLINE>｣
    [0].Test.Cat.concat                           ｢a y ｣
      [0].Test.Atom                               ｢a ｣
        [0].identifier                            ｢a｣
      [1].Test.Atom                               ｢y ｣
        [0].identifier                            ｢y｣
    [1].Test.Atom                                 ｢z<NEWLINE>｣
      [0].identifier                              ｢z｣
)");
    test::test_axe(*se, sa, "<: :> a\n", R"(
[0].Test                                          ｢<: :> a<NEWLINE>｣
  [0].Test.PrfLo.<:                               ｢<: :> a<NEWLINE>｣
    [0].Test.oper                                 ｢<:｣
    [1].Test.Args                                 ｢｣
    [2].Test.oper                                 ｢:>｣
    [3].Test.Atom                                 ｢a<NEWLINE>｣
      [0].identifier                              ｢a｣
)");
    test::test_axe(*se, sa, "<: 'foo' :> a\n", R"(
[0].Test                                          ｢<: 'foo' :> a<NEWLINE>｣
  [0].Test.PrfLo.<:                               ｢<: 'foo' :> a<NEWLINE>｣
    [0].Test.oper                                 ｢<:｣
    [1].Test.Args                                 ｢'foo' ｣
      [0].string                                  ｢'foo'｣
    [2].Test.oper                                 ｢:>｣
    [3].Test.Atom                                 ｢a<NEWLINE>｣
      [0].identifier                              ｢a｣
)");
    test::test_axe(*se, sa, "<: 'foo' , 'bar' , 'baz' :> a\n", R"(
[0].Test                                          ｢<: 'foo'  ... :> a<NEWLINE>｣
  [0].Test.PrfLo.<:                               ｢<: 'foo'  ... :> a<NEWLINE>｣
    [0].Test.oper                                 ｢<:｣
    [1].Test.Args                                 ｢'foo' , 'bar' , 'baz' ｣
      [0].string                                  ｢'foo'｣
      [1].string                                  ｢'bar'｣
      [2].string                                  ｢'baz'｣
    [2].Test.oper                                 ｢:>｣
    [3].Test.Atom                                 ｢a<NEWLINE>｣
      [0].identifier                              ｢a｣
)");
    test::test_axe(*se, sa, "a * <: 'foo' , 'bar' , 'baz' :> a\n", R"(
[0].Test                                          ｢a * < ... :> a<NEWLINE>｣
  [0].Test.Mul.*                                  ｢a * < ... :> a<NEWLINE>｣
    [0].Test.Atom                                 ｢a ｣
      [0].identifier                              ｢a｣
    [1].Test.oper                                 ｢*｣
    [2].Test.PrfLo.<:                             ｢<: 'foo'  ... :> a<NEWLINE>｣
      [0].Test.oper                               ｢<:｣
      [1].Test.Args                               ｢'foo' , 'bar' , 'baz' ｣
        [0].string                                ｢'foo'｣
        [1].string                                ｢'bar'｣
        [2].string                                ｢'baz'｣
      [2].Test.oper                               ｢:>｣
      [3].Test.Atom                               ｢a<NEWLINE>｣
        [0].identifier                            ｢a｣
)");
    test::test_axe(*se, sa, "{ b } a\n", R"(
[0].Test                                          ｢{ b } a<NEWLINE>｣
  [0].Test.PrfLo.{                                ｢{ b } a<NEWLINE>｣
    [0].Test.oper                                 ｢{｣
      [0].parenthesis                             ｢{｣
    [1].Test                                      ｢b ｣
      [0].Test.Atom                               ｢b ｣
        [0].identifier                            ｢b｣
    [2].Test.oper                                 ｢}｣
      [0].parenthesis                             ｢}｣
    [3].Test.Atom                                 ｢a<NEWLINE>｣
      [0].identifier                              ｢a｣
)");
    test::test_axe(*se, sa, "a { b } c\n", {none});
    test::test_axe(*se, sa, "a ( b ) c\n", R"(
[0].Test                                          ｢a ( b ) c<NEWLINE>｣
  [0].Test.Cat.concat                             ｢a ( b ) c<NEWLINE>｣
    [0].Test.Atom                                 ｢a ｣
      [0].identifier                              ｢a｣
    [1].Test.PrfHi.(                              ｢( b ) c<NEWLINE>｣
      [0].Test.oper                               ｢(｣
        [0].parenthesis                           ｢(｣
      [1].Test                                    ｢b ｣
        [0].Test.Atom                             ｢b ｣
          [0].identifier                          ｢b｣
      [2].Test.oper                               ｢)｣
        [0].parenthesis                           ｢)｣
      [3].Test.Atom                               ｢c<NEWLINE>｣
        [0].identifier                            ｢c｣
)");
    test::test_axe(*se, sa, "a << { b } c >>\n", R"(
[0].Test                                          ｢a <<  ... c >><NEWLINE>｣
  [0].Test.Cat.concat                             ｢a <<  ... c >><NEWLINE>｣
    [0].Test.Atom                                 ｢a ｣
      [0].identifier                              ｢a｣
    [1].Test.Atom                                 ｢<< {  ... c >><NEWLINE>｣
      [0].Test                                    ｢{ b } c ｣
        [0].Test.PrfLo.{                          ｢{ b } c ｣
          [0].Test.oper                           ｢{｣
            [0].parenthesis                       ｢{｣
          [1].Test                                ｢b ｣
            [0].Test.Atom                         ｢b ｣
              [0].identifier                      ｢b｣
          [2].Test.oper                           ｢}｣
            [0].parenthesis                       ｢}｣
          [3].Test.Atom                           ｢c ｣
            [0].identifier                        ｢c｣
)");
    test::test_axe(*se, sa, "a << b * c >>\n", {none});
    test::test_axe(*se, sa, "<< a { b } >> c\n", {none});
    test::test_axe(*se, sa, "a 1 a z\n", {none});
    test::test_axe(*se, sa, "a 1 + z\n", R"(
[0].Test                                          ｢a 1 + z<NEWLINE>｣
  [0].Test.Cat.concat                             ｢a 1 + z<NEWLINE>｣
    [0].Test.Cat.concat                           ｢a 1 + ｣
      [0].Test.Atom                               ｢a ｣
        [0].identifier                            ｢a｣
      [1].Test.Atom                               ｢1 + ｣
        [0].number                                ｢1｣
          [0].number.integer                      ｢1｣
            [0].number.integer.decimal            ｢1｣
              [0].number.plusMinus                ｢｣
              [1].number.unsigned.integer.decimal ｢1｣
    [1].Test.Atom                                 ｢z<NEWLINE>｣
      [0].identifier                              ｢z｣
)");
    test::test_axe(*se, sa, "a + b + c\n", R"(
[0].Test                                          ｢a + b + c<NEWLINE>｣
  [0].Test.Add.+                                  ｢a + b + c<NEWLINE>｣
    [0].Test.Atom                                 ｢a ｣
      [0].identifier                              ｢a｣
    [1].Test.oper                                 ｢+｣
    [2].Test.Atom                                 ｢b ｣
      [0].identifier                              ｢b｣
    [3].Test.oper                                 ｢+｣
    [4].Test.Atom                                 ｢c<NEWLINE>｣
      [0].identifier                              ｢c｣
)");
    test::test_axe(*se, sa, "a + b + c * d + e + f\n", R"(
[0].Test                                          ｢a + b ...  + f<NEWLINE>｣
  [0].Test.Add.+                                  ｢a + b ...  + f<NEWLINE>｣
    [0].Test.Atom                                 ｢a ｣
      [0].identifier                              ｢a｣
    [1].Test.oper                                 ｢+｣
    [2].Test.Atom                                 ｢b ｣
      [0].identifier                              ｢b｣
    [3].Test.oper                                 ｢+｣
    [4].Test.Mul.*                                ｢c * d ｣
      [0].Test.Atom                               ｢c ｣
        [0].identifier                            ｢c｣
      [1].Test.oper                               ｢*｣
      [2].Test.Atom                               ｢d ｣
        [0].identifier                            ｢d｣
    [5].Test.oper                                 ｢+｣
    [6].Test.Atom                                 ｢e ｣
      [0].identifier                              ｢e｣
    [7].Test.oper                                 ｢+｣
    [8].Test.Atom                                 ｢f<NEWLINE>｣
      [0].identifier                              ｢f｣
)");
    test::test_axe(*se, sa, "a + b + c - d - e + f + g\n", R"(
[0].Test                                          ｢a + b ...  + g<NEWLINE>｣
  [0].Test.Add.+                                  ｢a + b ...  + g<NEWLINE>｣
    [0].Test.Add.-                                ｢a + b ...  - e ｣
      [0].Test.Add.-                              ｢a + b ...  - d ｣
        [0].Test.Add.+                            ｢a + b + c ｣
          [0].Test.Atom                           ｢a ｣
            [0].identifier                        ｢a｣
          [1].Test.oper                           ｢+｣
          [2].Test.Atom                           ｢b ｣
            [0].identifier                        ｢b｣
          [3].Test.oper                           ｢+｣
          [4].Test.Atom                           ｢c ｣
            [0].identifier                        ｢c｣
        [1].Test.oper                             ｢-｣
        [2].Test.Atom                             ｢d ｣
          [0].identifier                          ｢d｣
      [1].Test.oper                               ｢-｣
      [2].Test.Atom                               ｢e ｣
        [0].identifier                            ｢e｣
    [1].Test.oper                                 ｢+｣
    [2].Test.Atom                                 ｢f ｣
      [0].identifier                              ｢f｣
    [3].Test.oper                                 ｢+｣
    [4].Test.Atom                                 ｢g<NEWLINE>｣
      [0].identifier                              ｢g｣
)");
    test::test_axe(*se, sa, "a - b + c + d - e\n", R"(
[0].Test                                          ｢a - b ...  - e<NEWLINE>｣
  [0].Test.Add.-                                  ｢a - b ...  - e<NEWLINE>｣
    [0].Test.Add.+                                ｢a - b ...  + d ｣
      [0].Test.Add.-                              ｢a - b ｣
        [0].Test.Atom                             ｢a ｣
          [0].identifier                          ｢a｣
        [1].Test.oper                             ｢-｣
        [2].Test.Atom                             ｢b ｣
          [0].identifier                          ｢b｣
      [1].Test.oper                               ｢+｣
      [2].Test.Atom                               ｢c ｣
        [0].identifier                            ｢c｣
      [3].Test.oper                               ｢+｣
      [4].Test.Atom                               ｢d ｣
        [0].identifier                            ｢d｣
    [1].Test.oper                                 ｢-｣
    [2].Test.Atom                                 ｢e<NEWLINE>｣
      [0].identifier                              ｢e｣
)");
    test::test_axe(*se, sa, "a + b + c - d + e + f\n", R"(
[0].Test                                          ｢a + b ...  + f<NEWLINE>｣
  [0].Test.Add.+                                  ｢a + b ...  + f<NEWLINE>｣
    [0].Test.Add.-                                ｢a + b ...  - d ｣
      [0].Test.Add.+                              ｢a + b + c ｣
        [0].Test.Atom                             ｢a ｣
          [0].identifier                          ｢a｣
        [1].Test.oper                             ｢+｣
        [2].Test.Atom                             ｢b ｣
          [0].identifier                          ｢b｣
        [3].Test.oper                             ｢+｣
        [4].Test.Atom                             ｢c ｣
          [0].identifier                          ｢c｣
      [1].Test.oper                               ｢-｣
      [2].Test.Atom                               ｢d ｣
        [0].identifier                            ｢d｣
    [1].Test.oper                                 ｢+｣
    [2].Test.Atom                                 ｢e ｣
      [0].identifier                              ｢e｣
    [3].Test.oper                                 ｢+｣
    [4].Test.Atom                                 ｢f<NEWLINE>｣
      [0].identifier                              ｢f｣
)");
    test::test_axe(*se, sa, "a % b = c = d % e\n", R"(
[0].Test                                          ｢a % b ...  % e<NEWLINE>｣
  [0].Test.Assign.%                               ｢a % b ...  % e<NEWLINE>｣
    [0].Test.Atom                                 ｢a ｣
      [0].identifier                              ｢a｣
    [1].Test.oper                                 ｢%｣
    [2].Test.Assign.=                             ｢b = c ...  % e<NEWLINE>｣
      [0].Test.Atom                               ｢b ｣
        [0].identifier                            ｢b｣
      [1].Test.oper                               ｢=｣
      [2].Test.Atom                               ｢c ｣
        [0].identifier                            ｢c｣
      [3].Test.oper                               ｢=｣
      [4].Test.Assign.%                           ｢d % e<NEWLINE>｣
        [0].Test.Atom                             ｢d ｣
          [0].identifier                          ｢d｣
        [1].Test.oper                             ｢%｣
        [2].Test.Atom                             ｢e<NEWLINE>｣
          [0].identifier                          ｢e｣
)");
    test::test_axe(*se, sa, "a = b = c % d = e = f\n", R"(
[0].Test                                          ｢a = b ...  = f<NEWLINE>｣
  [0].Test.Assign.=                               ｢a = b ...  = f<NEWLINE>｣
    [0].Test.Atom                                 ｢a ｣
      [0].identifier                              ｢a｣
    [1].Test.oper                                 ｢=｣
    [2].Test.Atom                                 ｢b ｣
      [0].identifier                              ｢b｣
    [3].Test.oper                                 ｢=｣
    [4].Test.Assign.%                             ｢c % d ...  = f<NEWLINE>｣
      [0].Test.Atom                               ｢c ｣
        [0].identifier                            ｢c｣
      [1].Test.oper                               ｢%｣
      [2].Test.Assign.=                           ｢d = e = f<NEWLINE>｣
        [0].Test.Atom                             ｢d ｣
          [0].identifier                          ｢d｣
        [1].Test.oper                             ｢=｣
        [2].Test.Atom                             ｢e ｣
          [0].identifier                          ｢e｣
        [3].Test.oper                             ｢=｣
        [4].Test.Atom                             ｢f<NEWLINE>｣
          [0].identifier                          ｢f｣
)");
  }
}
