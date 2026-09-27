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
}
