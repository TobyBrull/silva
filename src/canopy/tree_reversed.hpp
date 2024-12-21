#pragma once

#include "tree.hpp"

namespace silva {
  template<typename T = std::monostate>
  struct tree_reversed_t {
    struct node_t {
      index_t num_children   = 0;
      index_t children_begin = 0;
      [[no_unique_address]] T data{};
    };
    vector_t<node_t> nodes;

    bool is_consistent() const;

    enum class conversion_t {
      KEEP_CHILD_ORDER = 0,
      NAIVE            = 1,
    };
    tree_t<T> to_tree(conversion_t = conversion_t::KEEP_CHILD_ORDER) const;

    template<typename Visitor>
      requires std::invocable<Visitor, span_t<const tree_branch_t>, tree_event_t>
    expected_t<void> visit_subtree(Visitor, index_t start_node_index = 0) const;
  };
}

// IMPLEMENTATION

namespace silva {
}
