#pragma once

#include "canopy/tree.hpp"

#include "tokenization.hpp"

namespace silva {
  struct seed_engine_t;

  struct parse_tree_node_t : public tree_node_t {
    name_id_t rule_name = 0;
    index_t token_begin = std::numeric_limits<index_t>::max();
    index_t token_end   = std::numeric_limits<index_t>::min();

    index_t num_tokens() const { return token_end - token_begin; }

    friend auto operator<=>(const parse_tree_node_t&, const parse_tree_node_t&) = default;
  };

  struct parse_tree_t : public tree_t<parse_tree_node_t> {
    shared_ptr_t<const tokenization_t> tokenization;

    auto span(this auto&&);

    parse_tree_t subtree(index_t node_index) const;
  };

  struct parse_tree_span_t : tree_span_t<const parse_tree_node_t> {
    shared_ptr_t<const tokenization_t> tokenization;

    parse_tree_span_t sub_tree_span_at(index_t) const;

    enum class parse_tree_printing_t {
      ABSOLUTE,
      RELATIVE,
    };
    expected_t<string_t> to_string(index_t token_offset  = 50,
                                   parse_tree_printing_t = parse_tree_printing_t::ABSOLUTE);

    expected_t<string_t> to_graphviz();
  };

  // template<typename T>
  // struct bison_visitor {
  //   const ContextFreeGrammar* cfg = nullptr;
  //
  //   // Terminal-index ->
  //   // TerminalCalllback
  //   using TerminalCallback = std::function<void(T&)>;
  //   vector_t<TerminalCallback> terminal_callbacks;
  //
  //   // ProductionHandle ->
  //   // RHS index ->
  //   // ProductionCallback
  //   using ProductionCallback = std::function<void(T&, span_t<const T>)>;
  //   vector_t<vector_t<ProductionCallback>> production_callbacks;
  //
  //   bison_visitor(const ContextFreeGrammar*);
  //
  //   void set_terminal_callback(SymbolHandle, TerminalCallback);
  //   void set_production_callback(ProductionHandle, index_t rhs_index, ProductionCallback);
  //
  //   T visit_subtree(const ParseTree&) const;
  // };
}

// IMPLEMENTATION

namespace silva {
  inline auto parse_tree_t::span(this auto&& self)
  {
    return parse_tree_span_t{
        {
            .root   = &(self.nodes[0]),
            .stride = 1,
        },
        /* .tokenization = */ self.tokenization,
    };
  }

  inline parse_tree_span_t parse_tree_span_t::sub_tree_span_at(const index_t pos) const
  {
    return parse_tree_span_t{
        {
            .root   = &((*this)[pos]),
            .stride = stride,
        },
        /* .tokenization = */ tokenization,
    };
  }
}
