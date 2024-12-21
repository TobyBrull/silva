#pragma once

#include "types.hpp"

namespace silva {
  enum class tree_event_t {
    INVALID  = 0,
    ON_ENTRY = 0b01,
    ON_EXIT  = 0b10,
    ON_LEAF  = 0b11,
  };
  constexpr bool is_on_entry(tree_event_t);
  constexpr bool is_on_exit(tree_event_t);

  struct tree_branch_t {
    index_t node_index = 0;

    // This node ("node_index") is child number "child_index" of its parent. Zero for the root
    // node.
    index_t child_index = 0;
  };
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
