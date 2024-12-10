#pragma once

#include "canopy/assert.hpp"
#include "canopy/expected.hpp"
#include "canopy/types.hpp"

#include "tokenization.hpp"

#include <span>
#include <string>
#include <utility>
#include <vector>

namespace silva {
  struct parse_root_t;

  enum class parse_tree_event_t {
    INVALID  = 0,
    ON_ENTRY = 0b01,
    ON_EXIT  = 0b10,
    ON_LEAF  = 0b11,
  };
  bool is_on_entry(parse_tree_event_t);
  bool is_on_exit(parse_tree_event_t);

  struct parse_tree_t : public sprite_t {
    const_ptr_t<tokenization_t> tokenization;

    const_ptr_t<parse_root_t> root;

    struct node_t {
      index_t rule_index   = 0;
      index_t token_index  = 0;
      index_t num_children = 0;
      index_t children_end = 0;

      friend auto operator<=>(const node_t&, const node_t&) = default;
    };
    std::vector<node_t> nodes;

    bool is_consistent() const;

    struct visit_state_t {
      index_t node_index = 0;

      // This node ("node_index") is child number "child_index" of its parent. Zero for the root
      // node.
      index_t child_index = 0;
    };

    // Invokes the Visitor once (ON_LEAF for leaves) or twice (ON_ENTRY and ON_EXIT for non-leaves)
    // for each node. The visitation order is such that ON_ENTRY for a node happens immediately
    // before the first child invocations and ON_EXIT happens immediately after the last child
    // invokation. The "span" always contains a range that describes the path from the root node to
    // the current node.
    template<typename Visitor>
      requires std::invocable<Visitor, std::span<const visit_state_t>, parse_tree_event_t>
    expected_t<void> visit_subtree(Visitor, index_t start_node_index = 0) const;

    // Calls Visitor once for each child of "parent_node_index". The arguments are (1) the
    // node-index of the child node, "child_node_index", and (2) the "child_index", meaning that
    // "child_node_index" is child number "child_index" of "parent_node_index".
    template<typename Visitor>
      requires std::invocable<Visitor, index_t, index_t>
    expected_t<void> visit_children(Visitor, index_t parent_node_index) const;

    template<index_t N>
    expected_t<std::array<index_t, N>> get_children(index_t node_index) const;
  };

  std::string parse_tree_to_string(const parse_tree_t&, index_t token_offset = 50);
  std::string parse_tree_to_graphviz(const parse_tree_t&);

  // template<typename T>
  // struct bison_visitor {
  //   const ContextFreeGrammar* cfg = nullptr;
  //
  //   // Terminal-index ->
  //   // TerminalCalllback
  //   using TerminalCallback = std::function<void(T&)>;
  //   std::vector<TerminalCallback> terminal_callbacks;
  //
  //   // ProductionHandle ->
  //   // RHS index ->
  //   // ProductionCallback
  //   using ProductionCallback = std::function<void(T&, std::span<const T>)>;
  //   std::vector<std::vector<ProductionCallback>> production_callbacks;
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
  template<typename Visitor>
    requires std::
        invocable<Visitor, std::span<const parse_tree_t::visit_state_t>, parse_tree_event_t>
      expected_t<void> parse_tree_t::visit_subtree(Visitor visitor,
                                                   const index_t start_node_index) const
  {
    std::vector<visit_state_t> stack;
    const auto clean_stack_till = [&](const index_t new_node_index) -> expected_t<index_t> {
      index_t next_child_index = 0;
      while (!stack.empty() && nodes[stack.back().node_index].children_end <= new_node_index) {
        const index_t bi   = stack.back().node_index;
        const bool is_leaf = (nodes[bi].children_end == bi + 1);
        next_child_index   = stack.back().child_index + 1;
        if (!is_leaf) {
          SILVA_TRY(visitor(std::span<const visit_state_t>{stack}, parse_tree_event_t::ON_EXIT));
        }
        stack.pop_back();
      }
      return next_child_index;
    };

    const index_t end_node_index = nodes[start_node_index].children_end;
    for (index_t node_index = start_node_index; node_index < end_node_index; ++node_index) {
      const index_t new_child_index = SILVA_TRY(clean_stack_till(node_index));
      stack.push_back({.node_index = node_index, .child_index = new_child_index});
      const bool is_leaf = (nodes[node_index].children_end == node_index + 1);
      if (is_leaf) {
        SILVA_TRY(visitor(std::span<const visit_state_t>{stack}, parse_tree_event_t::ON_LEAF));
      }
      else {
        SILVA_TRY(visitor(std::span<const visit_state_t>{stack}, parse_tree_event_t::ON_ENTRY));
      }
    }
    const index_t new_child_index = SILVA_TRY(clean_stack_till(end_node_index));
    SILVA_ASSERT(new_child_index == 1);
    SILVA_ASSERT(stack.empty());
    return {};
  }

  template<typename Visitor>
    requires std::invocable<Visitor, index_t, index_t>
  expected_t<void> parse_tree_t::visit_children(Visitor visitor,
                                                const index_t parent_node_index) const
  {
    const node_t& node       = nodes[parent_node_index];
    index_t child_node_index = parent_node_index + 1;
    for (index_t child_index = 0; child_index < node.num_children; ++child_index) {
      const bool cont = SILVA_TRY(visitor(child_node_index, child_index));
      if (!cont) {
        break;
      }
      child_node_index = nodes[child_node_index].children_end;
    }
    return {};
  }

  template<index_t N>
  expected_t<std::array<index_t, N>> parse_tree_t::get_children(index_t node_index) const
  {
    const node_t& node = nodes[node_index];
    SILVA_EXPECT(node.num_children == N);
    std::array<index_t, N> retval;
    SILVA_TRY(visit_children(
        [&](const index_t child_node_index, const index_t child_num) -> expected_t<bool> {
          retval[child_num] = child_node_index;
          return true;
        },
        node_index));
    return {std::move(retval)};
  }
}
