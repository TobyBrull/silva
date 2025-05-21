#include "seed_axe.hpp"

#include "syntax.hpp"
#include "syntax/parse_tree_nursery.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;
using namespace silva::impl;
using enum silva::impl::assoc_t;
using enum silva::token_category_t;

namespace silva::test {
  template<typename SeedAxeNursery>
  expected_t<parse_tree_ptr_t>
  run_seed_axe(syntax_ward_t& sw, const seed_axe_t& seed_axe, tokenization_ptr_t tp)
  {
    const index_t n = tp->tokens.size();
    SeedAxeNursery nursery(seed_axe, std::move(tp));
    const parse_tree_node_t sub = SILVA_EXPECT_FWD(nursery.expression());
    SILVA_EXPECT(sub.num_children == 1, ASSERT);
    SILVA_EXPECT(sub.subtree_size == nursery.tree.size(), ASSERT);
    SILVA_EXPECT(nursery.token_index == n, MAJOR, "Tokens left after parsing fern.");
    return sw.add(std::move(nursery).finish());
  }

  template<typename SeedAxeNursery>
  void test_seed_axe(syntax_ward_ptr_t swp,
                     const seed_axe_t& pa,
                     const string_view_t text,
                     const optional_t<string_view_t> expected_str)
  {
    INFO(text);
    auto maybe_tt = tokenize(swp, "", string_t{text});
    REQUIRE(maybe_tt.has_value());
    auto tt              = std::move(maybe_tt).value();
    auto maybe_result_pt = run_seed_axe<SeedAxeNursery>(*swp, pa, std::move(tt));
    optional_t<string_t> result_str;
    if (maybe_result_pt.has_value()) {
      auto result_pt = std::move(maybe_result_pt).value();
      result_str     = SILVA_EXPECT_REQUIRE(result_pt->span().to_string());
      UNSCOPED_INFO(result_str.value());
    }
    else {
      UNSCOPED_INFO(to_string(maybe_result_pt.error()).as_string_view());
    }
    REQUIRE(maybe_result_pt.has_value() == expected_str.has_value());
    if (!expected_str.has_value()) {
      return;
    }
    CHECK(result_str.value() == expected_str.value().substr(1));
  }

  TEST_CASE("seed-axe-basic", "[seed_axe_t]")
  {
    struct test_nursery_t : public parse_tree_nursery_t {
      const seed_axe_t& seed_axe;

      const name_id_t ni_atom = swp->name_id_of("Test", "Atom");
      const name_id_t ni_expr = swp->name_id_of("Expr");

      test_nursery_t(const seed_axe_t& seed_axe, tokenization_ptr_t tp)
        : parse_tree_nursery_t(tp), seed_axe(seed_axe)
      {
      }

      expected_t<parse_tree_node_t> atom()
      {
        auto ss_rule = stake();
        ss_rule.create_node(ni_atom);
        SILVA_EXPECT(num_tokens_left() >= 1, MINOR, "No token left for atom expression");
        SILVA_EXPECT(token_data_by()->category == NUMBER || token_data_by()->category == IDENTIFIER,
                     MINOR);
        token_index += 1;
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> any_rule(const name_id_t rule_name)
      {
        if (rule_name == ni_atom) {
          return atom();
        }
        else if (rule_name == ni_expr) {
          return expression();
        }
        else {
          SILVA_EXPECT(false, MAJOR, "unexpected rule {}", swp->name_id_wrap(rule_name));
        }
      }

      expected_t<parse_tree_node_t> expression()
      {
        const auto dg = seed_axe_t::parse_delegate_t::make<&test_nursery_t::any_rule>(this);
        return seed_axe.apply(*this, dg);
      }
    };

    syntax_ward_t sw;
    const string_view_t test_seed_axe = R"'( _.Test.Atom [
        - Nst   = nest  atom_nest '(' ')'
        - Dot   = rtl   infix '.'
        - Sub   = ltr   postfix_nest '[' ']'
        - Dol   = ltr   postfix '$'
        - Exc   = ltr   postfix '!'
        - Til   = rtl   prefix '~'
        - Prf   = rtl   prefix '+' '-'
        - Mul   = ltr   infix '*' '/'
        - Add   = ltr   infix '+' '-'
        - Ter   = rtl   ternary '?' ':'
        - Eqa   = rtl   infix '='
      ] )'";
    const auto tt = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "test.seed-axe", test_seed_axe));
    const auto se = standard_seed_engine(sw.ptr());
    const auto pt = SILVA_EXPECT_REQUIRE(se->apply(tt, sw.name_id_of("Seed", "Axe")));
    const auto sa =
        SILVA_EXPECT_REQUIRE(seed_axe_create(sw.ptr(), sw.name_id_of("Expr"), pt->span()));
    CHECK(!sa.concat_result.has_value());
    CHECK(sa.results.size() == 15);
    CHECK(sa.results.at(*sw.token_id("=")) ==
          seed_axe_result_t{
              .prefix = none,
              .regular =
                  result_oper_t<oper_regular_t>{
                      .oper       = infix_t{*sw.token_id("=")},
                      .name       = sw.name_id_of("Expr", "Eqa", "="),
                      .precedence = precedence_t{.level_index = 1, .assoc = RIGHT_TO_LEFT},
                      .pts        = pt->span().sub_tree_span_at(76),
                  },
              .is_right_bracket = false,
          });
    CHECK(sa.results.at(*sw.token_id("?")) ==
          seed_axe_result_t{
              .prefix = none,
              .regular =
                  result_oper_t<oper_regular_t>{
                      .oper       = ternary_t{*sw.token_id("?"), *sw.token_id(":")},
                      .name       = sw.name_id_of("Expr", "Ter", "?"),
                      .precedence = precedence_t{.level_index = 2, .assoc = RIGHT_TO_LEFT},
                      .pts        = pt->span().sub_tree_span_at(69),
                  },
              .is_right_bracket = false,
          });
    CHECK(sa.results.at(*sw.token_id(":")) ==
          seed_axe_result_t{
              .prefix           = none,
              .regular          = none,
              .is_right_bracket = true,
          });
    CHECK(sa.results.at(*sw.token_id("+")) ==
          seed_axe_result_t{
              .prefix =
                  result_oper_t<oper_prefix_t>{
                      .oper       = prefix_t{*sw.token_id("+")},
                      .name       = sw.name_id_of("Expr", "Prf", "+"),
                      .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                      .pts        = pt->span().sub_tree_span_at(48),
                  },
              .regular =
                  result_oper_t<oper_regular_t>{
                      .oper       = infix_t{*sw.token_id("+")},
                      .name       = sw.name_id_of("Expr", "Add", "+"),
                      .precedence = precedence_t{.level_index = 3, .assoc = LEFT_TO_RIGHT},
                      .pts        = pt->span().sub_tree_span_at(62),
                  },
              .is_right_bracket = false,
          });
    CHECK(sa.results.at(*sw.token_id("-")) ==
          seed_axe_result_t{
              .prefix =
                  result_oper_t<oper_prefix_t>{
                      .oper       = prefix_t{*sw.token_id("-")},
                      .name       = sw.name_id_of("Expr", "Prf", "-"),
                      .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                      .pts        = pt->span().sub_tree_span_at(49),
                  },
              .regular =
                  result_oper_t<oper_regular_t>{
                      .oper       = infix_t{*sw.token_id("-")},
                      .name       = sw.name_id_of("Expr", "Add", "-"),
                      .precedence = precedence_t{.level_index = 3, .assoc = LEFT_TO_RIGHT},
                      .pts        = pt->span().sub_tree_span_at(63),
                  },
              .is_right_bracket = false,
          });
    CHECK(sa.results.at(*sw.token_id("(")) ==
          seed_axe_result_t{
              .prefix =
                  result_oper_t<oper_prefix_t>{
                      .oper       = atom_nest_t{*sw.token_id("("), *sw.token_id(")")},
                      .name       = sw.name_id_of("Expr", "Nst", "("),
                      .precedence = precedence_t{.level_index = 11, .assoc = NEST},
                      .pts        = pt->span().sub_tree_span_at(10),
                  },
              .regular          = none,
              .is_right_bracket = false,
          });
    CHECK(sa.results.at(*sw.token_id(")")) ==
          seed_axe_result_t{
              .prefix           = none,
              .regular          = none,
              .is_right_bracket = true,
          });

    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "1", R"(
[0]_.Test.Atom                                    1
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "1 + 2", R"(
[0]_.Expr.Add.+                                   1 + 2
  [0]_.Test.Atom                                  1
  [1]_.Test.Atom                                  2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "1 - 2", R"(
[0]_.Expr.Add.-                                   1 - 2
  [0]_.Test.Atom                                  1
  [1]_.Test.Atom                                  2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "1 + 2 * 3 + 4", R"(
[0]_.Expr.Add.+                                   1 + ... + 4
  [0]_.Expr.Add.+                                 1 + 2 * 3
    [0]_.Test.Atom                                1
    [1]_.Expr.Mul.*                               2 * 3
      [0]_.Test.Atom                              2
      [1]_.Test.Atom                              3
  [1]_.Test.Atom                                  4
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(),
                                        sa,
                                        "1 - 2 + f . g . h * 3 / 4",
                                        R"(
[0]_.Expr.Add.+                                   1 - ... / 4
  [0]_.Expr.Add.-                                 1 - 2
    [0]_.Test.Atom                                1
    [1]_.Test.Atom                                2
  [1]_.Expr.Mul./                                 f . ... / 4
    [0]_.Expr.Mul.*                               f . ... * 3
      [0]_.Expr.Dot..                             f . g . h
        [0]_.Test.Atom                            f
        [1]_.Expr.Dot..                           g . h
          [0]_.Test.Atom                          g
          [1]_.Test.Atom                          h
      [1]_.Test.Atom                              3
    [1]_.Test.Atom                                4
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "2 ! + 3", R"(
[0]_.Expr.Add.+                                   2 ! + 3
  [0]_.Expr.Exc.!                                 2 !
    [0]_.Test.Atom                                2
  [1]_.Test.Atom                                  3
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, " - + 1", R"(
[0]_.Expr.Prf.-                                   - + 1
  [0]_.Expr.Prf.+                                 + 1
    [0]_.Test.Atom                                1
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a + - + 1", R"(
[0]_.Expr.Add.+                                   a + - + 1
  [0]_.Test.Atom                                  a
  [1]_.Expr.Prf.-                                 - + 1
    [0]_.Expr.Prf.+                               + 1
      [0]_.Test.Atom                              1
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "- - 1 * 2", R"(
[0]_.Expr.Mul.*                                   - - 1 * 2
  [0]_.Expr.Prf.-                                 - - 1
    [0]_.Expr.Prf.-                               - 1
      [0]_.Test.Atom                              1
  [1]_.Test.Atom                                  2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "- - 1 . 2", R"(
[0]_.Expr.Prf.-                                   - - 1 . 2
  [0]_.Expr.Prf.-                                 - 1 . 2
    [0]_.Expr.Dot..                               1 . 2
      [0]_.Test.Atom                              1
      [1]_.Test.Atom                              2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "1 . 2 !", R"(
[0]_.Expr.Exc.!                                   1 . 2 !
  [0]_.Expr.Dot..                                 1 . 2
    [0]_.Test.Atom                                1
    [1]_.Test.Atom                                2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "1 + 2 !", R"(
[0]_.Expr.Add.+                                   1 + 2 !
  [0]_.Test.Atom                                  1
  [1]_.Expr.Exc.!                                 2 !
    [0]_.Test.Atom                                2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "2 ! . 3", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "2 . - 3", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "2 $ !", R"(
[0]_.Expr.Exc.!                                   2 $ !
  [0]_.Expr.Dol.$                                 2 $
    [0]_.Test.Atom                                2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "2 ! $", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "+ ~ 2", R"(
[0]_.Expr.Prf.+                                   + ~ 2
  [0]_.Expr.Til.~                                 ~ 2
    [0]_.Test.Atom                                2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "~ + 2", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "( ( 0 ) )", R"(
[0]_.Expr.Nst.(                                   ( ( 0 ) )
  [0]_.Expr.Nst.(                                 ( 0 )
    [0]_.Test.Atom                                0
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "1 * ( 2 + 3 ) * 4", R"(
[0]_.Expr.Mul.*                                   1 * ... * 4
  [0]_.Expr.Mul.*                                 1 * ... 3 )
    [0]_.Test.Atom                                1
    [1]_.Expr.Nst.(                               ( 2 + 3 )
      [0]_.Expr.Add.+                             2 + 3
        [0]_.Test.Atom                            2
        [1]_.Test.Atom                            3
  [1]_.Test.Atom                                  4
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "1 * ( 2 + 3 ) * 4", R"(
[0]_.Expr.Mul.*                                   1 * ... * 4
  [0]_.Expr.Mul.*                                 1 * ... 3 )
    [0]_.Test.Atom                                1
    [1]_.Expr.Nst.(                               ( 2 + 3 )
      [0]_.Expr.Add.+                             2 + 3
        [0]_.Test.Atom                            2
        [1]_.Test.Atom                            3
  [1]_.Test.Atom                                  4
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a [ 0 ]", R"(
[0]_.Expr.Sub.[                                   a [ 0 ]
  [0]_.Test.Atom                                  a
  [1]_.Test.Atom                                  0
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a [ 0 ] [ 1 ]", R"(
[0]_.Expr.Sub.[                                   a [ ... 1 ]
  [0]_.Expr.Sub.[                                 a [ 0 ]
    [0]_.Test.Atom                                a
    [1]_.Test.Atom                                0
  [1]_.Test.Atom                                  1
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a [ 0 ] . b [ 1 ]", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a [ 0 ] + b [ 1 ]", R"(
[0]_.Expr.Add.+                                   a [ ... 1 ]
  [0]_.Expr.Sub.[                                 a [ 0 ]
    [0]_.Test.Atom                                a
    [1]_.Test.Atom                                0
  [1]_.Expr.Sub.[                                 b [ 1 ]
    [0]_.Test.Atom                                b
    [1]_.Test.Atom                                1
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a ? b : c", R"(
[0]_.Expr.Ter.?                                   a ? b : c
  [0]_.Test.Atom                                  a
  [1]_.Test.Atom                                  b
  [2]_.Test.Atom                                  c
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a ? b : c ? d : e", R"(
[0]_.Expr.Ter.?                                   a ? ... : e
  [0]_.Test.Atom                                  a
  [1]_.Test.Atom                                  b
  [2]_.Expr.Ter.?                                 c ? d : e
    [0]_.Test.Atom                                c
    [1]_.Test.Atom                                d
    [2]_.Test.Atom                                e
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a ? b ? c : d : e", R"(
[0]_.Expr.Ter.?                                   a ? ... : e
  [0]_.Test.Atom                                  a
  [1]_.Expr.Ter.?                                 b ? c : d
    [0]_.Test.Atom                                b
    [1]_.Test.Atom                                c
    [2]_.Test.Atom                                d
  [2]_.Test.Atom                                  e
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a = b ? c = d : e = f", R"(
[0]_.Expr.Eqa.=                                   a = ... = f
  [0]_.Test.Atom                                  a
  [1]_.Expr.Eqa.=                                 b ? ... = f
    [0]_.Expr.Ter.?                               b ? ... : e
      [0]_.Test.Atom                              b
      [1]_.Expr.Eqa.=                             c = d
        [0]_.Test.Atom                            c
        [1]_.Test.Atom                            d
      [2]_.Test.Atom                              e
    [1]_.Test.Atom                                f
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a + b ? c + d : e + f", R"(
[0]_.Expr.Ter.?                                   a + ... + f
  [0]_.Expr.Add.+                                 a + b
    [0]_.Test.Atom                                a
    [1]_.Test.Atom                                b
  [1]_.Expr.Add.+                                 c + d
    [0]_.Test.Atom                                c
    [1]_.Test.Atom                                d
  [2]_.Expr.Add.+                                 e + f
    [0]_.Test.Atom                                e
    [1]_.Test.Atom                                f
)");
  }

  TEST_CASE("seed-axe-advanced", "[seed_axe_t]")
  {
    struct test_nursery_t : public parse_tree_nursery_t {
      const seed_axe_t& seed_axe;

      const token_id_t ti_comma = swp->token_id(",").value();

      const name_id_t ni_expr = swp->name_id_of("Expr");
      const name_id_t ni_atom = swp->name_id_of("Test", "Atom");
      const name_id_t ni_arg  = swp->name_id_of("Test", "Arg");
      const name_id_t ni_args = swp->name_id_of("Test", "Args");

      test_nursery_t(const seed_axe_t& seed_axe, tokenization_ptr_t tp)
        : parse_tree_nursery_t(tp), seed_axe(seed_axe)
      {
      }

      expected_t<parse_tree_node_t> atom()
      {
        auto ss_rule = stake();
        ss_rule.create_node(ni_atom);
        SILVA_EXPECT(num_tokens_left() >= 1, MINOR, "No token left for atom expression");
        if (token_data_by()->category == NUMBER) {
          SILVA_EXPECT(num_tokens_left() >= 2 && token_data_by(1)->category == OPERATOR, MINOR);
          token_index += 2;
        }
        else {
          SILVA_EXPECT(token_data_by()->category == IDENTIFIER, MINOR);
          token_index += 1;
        }
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> arg()
      {
        auto ss_rule = stake();
        ss_rule.create_node(ni_arg);
        SILVA_EXPECT_PARSE(ni_arg, token_data_by()->category == STRING, "expected string");
        token_index += 1;
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> args()
      {
        auto ss_rule = stake();
        ss_rule.create_node(ni_args);
        bool first = true;
        while (num_tokens_left() >= 1) {
          if (!first) {
            if (token_id_by() == ti_comma) {
              token_index += 1;
              ss_rule.add_proto_node(SILVA_EXPECT_FWD(arg()));
            }
            else {
              break;
            }
          }
          else {
            auto res = arg();
            if (!res.has_value()) {
              break;
            }
            ss_rule.add_proto_node(std::move(res).value());
          }
          first = false;
        }
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> any_rule(const name_id_t rule_name)
      {
        if (rule_name == ni_atom) {
          return atom();
        }
        else if (rule_name == ni_expr) {
          return expression();
        }
        else if (rule_name == ni_args) {
          return args();
        }
        else {
          SILVA_EXPECT(false, MAJOR, "unexpected rule {}", swp->name_id_wrap(rule_name));
        }
      }

      expected_t<parse_tree_node_t> expression()
      {
        const auto dg = seed_axe_t::parse_delegate_t::make<&test_nursery_t::any_rule>(this);
        return seed_axe.apply(*this, dg);
      }
    };

    syntax_ward_t sw;
    const string_view_t test_seed_axe = R"'( _.Test.Atom [
        - Nst     = nest  atom_nest_transparent '<<' '>>'
        - Prf_hi  = rtl   prefix_nest '(' ')'
        - Cat     = ltr   infix concat
        - Prf_lo  = rtl   prefix_nest '{' '}'
                          prefix_nest -> _.Test.Args '<:' ':>'
        - Mul     = ltr   infix '*'
        - Add     = ltr   infix_flat '+' infix '-'
        - Assign  = rtl   infix_flat '=' infix '%'
      ] )'";
    const auto tt = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "test.seed-axe", test_seed_axe));
    const auto se = standard_seed_engine(sw.ptr());
    const auto pt = SILVA_EXPECT_REQUIRE(se->apply(tt, sw.name_id_of("Seed", "Axe")));
    const auto sa =
        SILVA_EXPECT_REQUIRE(seed_axe_create(sw.ptr(), sw.name_id_of("Expr"), pt->span()));
    CHECK(sa.concat_result.has_value());
    CHECK(sa.results.size() == 13);

    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "x", R"(
[0]_.Test.Atom                                    x
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "x y z", R"(
[0]_.Expr.Cat.concat                              x y z
  [0]_.Expr.Cat.concat                            x y
    [0]_.Test.Atom                                x
    [1]_.Test.Atom                                y
  [1]_.Test.Atom                                  z
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "<: :> a", R"(
[0]_.Expr.Prf_lo.<:                               <: :> a
  [0]_.Test.Args                                  
  [1]_.Test.Atom                                  a
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "<: 'foo' :> a", R"(
[0]_.Expr.Prf_lo.<:                               <: 'foo' :> a
  [0]_.Test.Args                                  'foo'
    [0]_.Test.Arg                                 'foo'
  [1]_.Test.Atom                                  a
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "<: 'foo' , 'bar' , 'baz' :> a", R"(
[0]_.Expr.Prf_lo.<:                               <: 'foo' ... :> a
  [0]_.Test.Args                                  'foo' , 'bar' , 'baz'
    [0]_.Test.Arg                                 'foo'
    [1]_.Test.Arg                                 'bar'
    [2]_.Test.Arg                                 'baz'
  [1]_.Test.Atom                                  a
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "x * <: 'foo' , 'bar' , 'baz' :> a", R"(
[0]_.Expr.Mul.*                                   x * ... :> a
  [0]_.Test.Atom                                  x
  [1]_.Expr.Prf_lo.<:                             <: 'foo' ... :> a
    [0]_.Test.Args                                'foo' , 'bar' , 'baz'
      [0]_.Test.Arg                               'foo'
      [1]_.Test.Arg                               'bar'
      [2]_.Test.Arg                               'baz'
    [1]_.Test.Atom                                a
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "{ b } a", R"(
[0]_.Expr.Prf_lo.{                                { b } a
  [0]_.Test.Atom                                  b
  [1]_.Test.Atom                                  a
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a { b } c", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a ( b ) c", R"(
[0]_.Expr.Cat.concat                              a ( b ) c
  [0]_.Test.Atom                                  a
  [1]_.Expr.Prf_hi.(                              ( b ) c
    [0]_.Test.Atom                                b
    [1]_.Test.Atom                                c
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a << { b } c >>", R"(
[0]_.Expr.Cat.concat                              a << ... c >>
  [0]_.Test.Atom                                  a
  [1]_.Expr.Prf_lo.{                              { b } c
    [0]_.Test.Atom                                b
    [1]_.Test.Atom                                c
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "<< a { b } >> c", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "x 1 x z", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "x 1 { z", R"(
[0]_.Expr.Cat.concat                              x 1 { z
  [0]_.Expr.Cat.concat                            x 1 {
    [0]_.Test.Atom                                x
    [1]_.Test.Atom                                1 {
  [1]_.Test.Atom                                  z
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a + b + c", R"(
[0]_.Expr.Add.+                                   a + b + c
  [0]_.Test.Atom                                  a
  [1]_.Test.Atom                                  b
  [2]_.Test.Atom                                  c
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a + b + c * d + e + f", R"(
[0]_.Expr.Add.+                                   a + ... + f
  [0]_.Test.Atom                                  a
  [1]_.Test.Atom                                  b
  [2]_.Expr.Mul.*                                 c * d
    [0]_.Test.Atom                                c
    [1]_.Test.Atom                                d
  [3]_.Test.Atom                                  e
  [4]_.Test.Atom                                  f
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a + b + c - d - e + f + g", R"(
[0]_.Expr.Add.+                                   a + ... + g
  [0]_.Expr.Add.-                                 a + ... - e
    [0]_.Expr.Add.-                               a + ... - d
      [0]_.Expr.Add.+                             a + b + c
        [0]_.Test.Atom                            a
        [1]_.Test.Atom                            b
        [2]_.Test.Atom                            c
      [1]_.Test.Atom                              d
    [1]_.Test.Atom                                e
  [1]_.Test.Atom                                  f
  [2]_.Test.Atom                                  g
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a - b + c + d - e", R"(
[0]_.Expr.Add.-                                   a - ... - e
  [0]_.Expr.Add.+                                 a - ... + d
    [0]_.Expr.Add.-                               a - b
      [0]_.Test.Atom                              a
      [1]_.Test.Atom                              b
    [1]_.Test.Atom                                c
    [2]_.Test.Atom                                d
  [1]_.Test.Atom                                  e
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a + b + c - d + e + f", R"(
[0]_.Expr.Add.+                                   a + ... + f
  [0]_.Expr.Add.-                                 a + ... - d
    [0]_.Expr.Add.+                               a + b + c
      [0]_.Test.Atom                              a
      [1]_.Test.Atom                              b
      [2]_.Test.Atom                              c
    [1]_.Test.Atom                                d
  [1]_.Test.Atom                                  e
  [2]_.Test.Atom                                  f
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a % b = c = d % e", R"(
[0]_.Expr.Assign.%                                a % ... % e
  [0]_.Test.Atom                                  a
  [1]_.Expr.Assign.=                              b = ... % e
    [0]_.Test.Atom                                b
    [1]_.Test.Atom                                c
    [2]_.Expr.Assign.%                            d % e
      [0]_.Test.Atom                              d
      [1]_.Test.Atom                              e
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), sa, "a = b = c % d = e = f", R"(
[0]_.Expr.Assign.=                                a = ... = f
  [0]_.Test.Atom                                  a
  [1]_.Test.Atom                                  b
  [2]_.Expr.Assign.%                              c % ... = f
    [0]_.Test.Atom                                c
    [1]_.Expr.Assign.=                            d = e = f
      [0]_.Test.Atom                              d
      [1]_.Test.Atom                              e
      [2]_.Test.Atom                              f
)");
  }
}
