#pragma once

#include "types.hpp"

namespace silva {
  struct tree_node_t {
    // Number of direct children of this node.
    index_t num_children = 0;

    // Size of the subtree rooted in this node, including this node.
    index_t subtree_size = 1;

    friend auto operator<=>(const tree_node_t&, const tree_node_t&) = default;
  };

  struct tree_branch_t {
    index_t node_index = 0;

    // This node ("node_index") is child number "child_index" of its parent. Zero for the root
    // node.
    index_t child_index = 0;
  };

  enum class tree_event_t {
    INVALID  = 0,
    ON_ENTRY = 0b01,
    ON_EXIT  = 0b10,
    ON_LEAF  = 0b11,
  };
  constexpr bool is_on_entry(tree_event_t);
  constexpr bool is_on_exit(tree_event_t);
}

// IMPLEMENTATION

namespace silva {
  constexpr bool is_on_entry(const tree_event_t event)
  {
    const auto retval = (to_int(event) & to_int(tree_event_t::ON_ENTRY));
    return retval != 0;
  }

  constexpr bool is_on_exit(const tree_event_t event)
  {
    const auto retval = (to_int(event) & to_int(tree_event_t::ON_EXIT));
    return retval != 0;
  }
}
