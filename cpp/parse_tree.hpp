#pragma once

#include "canopy/tree.hpp"

#include "tokenization.hpp"

namespace silva {
  struct parse_root_t;

  struct parse_tree_node_data_t {
    full_name_id_t rule_name = 0;
    index_t token_begin      = std::numeric_limits<index_t>::max();
    index_t token_end        = std::numeric_limits<index_t>::min();

    friend auto operator<=>(const parse_tree_node_data_t&, const parse_tree_node_data_t&) = default;
  };

  struct parse_tree_t : public tree_t<parse_tree_node_data_t> {
    shared_ptr_t<const tokenization_t> tokenization;

    parse_tree_t subtree(index_t node_index) const;
  };

  full_name_id_style_t parse_tree_full_name_style(token_context_ptr_t);

  expected_t<string_t> parse_tree_to_string(const parse_tree_t&, index_t token_offset = 50);
  expected_t<string_t> parse_tree_to_graphviz(const parse_tree_t&);

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
}
