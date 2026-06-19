#pragma once

#include "seed.lexicon.hpp"
#include "seed_axe.impl.hpp"

#include "parse_tree_nursery.hpp"

#include "canopy/delegate.hpp"

namespace silva::seed {

  // An mechanism for parsing [a]rithmetic e[x]pr[e]ssions. This is a version of the Shunting Yard
  // algorithm or precedence climbing.
  //
  // * https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
  //
  // You basically specify a number of operators, in the order of their desired precedence (from
  // high precedence to low precedence), what type of operator they are (prefix, infx,
  // left-to-right, right-to-left, nested, ...), and what an atom looks like. From this description
  // an expression parser is then generated. For example,
  //
  // « number [
  //   - Prefix   = ltr   prefix '-'
  //   - Product  = ltr   infix '*' '/'
  //   - Addition = ltr   infix '+' '-'
  // ] »
  //
  // can be used to parse
  //
  // « 1 + 2 * 3 + - 4 »
  //
  // in the normal, mathematical way.

  const string_view_t axe_str = R"'(
Seed.Axe:
  ⊙ = Seed.Nonterminal Seed.Nonterminal newline indent ( Level newline ) * dedent
  Level = rule_name '=' Assoc Ops *
  Assoc = 'ltr' | 'rtl'
  Ops = OpType ( '->' Seed.Nonterminal ) ? Op *
  OpType = ( 'prefix_nest' | 'prefix'
           | 'infix_flat' | 'infix' | 'ternary'
           | 'postfix_nest' | 'postfix' )
  Op = string | 'concat'
)'";

  struct axe_t {
    lexicon_ptr_t lp;
    name_id_t name;
    name_id_ref_t atom_rule;
    name_id_ref_t oper_rule;
    hash_map_t<token_id_t, impl::axe_result_t> results;
    optional_t<impl::result_oper_t<impl::oper_regular_t>> concat_result;

    hash_map_t<name_id_t, impl::level_index_t> level_map;

    void compile_reset();
    template<Namespace Ns>
    expected_t<void> compile(const lexicon_t&, const Ns&);

    using parse_delegate_t = delegate_t<expected_t<parse_tree_node_t>(name_id_t)>;
    expected_t<parse_tree_node_t> apply(parse_tree_nursery_t&, name_id_t, parse_delegate_t) const;
  };

  expected_t<axe_t> axe_create(syntax_farm_ptr_t, name_id_t axe_name, parse_tree_span_t);
}

// IMPLEMENTATION

namespace silva::seed {
  namespace impl {
    template<typename Oper, typename F>
    expected_t<void> for_each_name_id_ref(result_oper_t<Oper>& oo, F f)
    {
      const auto visitor = [&]<typename InnerOper>(InnerOper& inner_oper) -> expected_t<void> {
        if constexpr (std::same_as<InnerOper, prefix_nest_t> ||
                      std::same_as<InnerOper, ternary_t> ||
                      std::same_as<InnerOper, postfix_nest_t>) {
          if (inner_oper.nest_rule_name.has_value()) {
            SILVA_EXPECT_FWD(f(inner_oper.nest_rule_name.value()));
          }
        }
        return {};
      };
      return std::visit(visitor, oo.oper);
    }

    template<typename F>
    expected_t<void> for_each_name_id_ref(axe_t& aa, F f)
    {
      if (aa.concat_result.has_value()) {
        SILVA_EXPECT_FWD(for_each_name_id_ref(aa.concat_result.value(), f));
      }
      for (auto& [tid, res]: aa.results) {
        if (res.prefix.has_value()) {
          SILVA_EXPECT_FWD(for_each_name_id_ref(res.prefix.value(), f));
        }
        if (res.regular.has_value()) {
          SILVA_EXPECT_FWD(for_each_name_id_ref(res.regular.value(), f));
        }
      }
      return {};
    }
  }

  template<Namespace Ns>
  expected_t<void> axe_t::compile(const lexicon_t& lexicon, const Ns& ns)
  {
    SILVA_EXPECT_FWD(atom_rule.resolve(name, lexicon, ns));
    SILVA_EXPECT_FWD(oper_rule.resolve(name, lexicon, ns));
    return impl::for_each_name_id_ref(*this, [&](name_id_ref_t& nir) -> expected_t<void> {
      return nir.resolve(name, lexicon, ns);
    });
  }
}
