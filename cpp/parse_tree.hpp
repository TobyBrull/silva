#pragma once

#include "canopy/tree.hpp"

#include "tokenization.hpp"

namespace silva {
  struct parse_root_t;

  struct parse_tree_node_data_t {
    full_name_id_t rule_name = 0;
    index_t token_begin      = 0;
    index_t token_end        = 0;

    friend auto operator<=>(const parse_tree_node_data_t&, const parse_tree_node_data_t&) = default;
  };

  struct parse_tree_t : public tree_t<parse_tree_node_data_t> {
    shared_ptr_t<const tokenization_t> tokenization;
  };

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
