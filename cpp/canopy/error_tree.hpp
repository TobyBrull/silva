#pragma once

#include "any_vector.hpp"
#include "assert.hpp"
#include "tree_types.hpp"

namespace silva {
  enum class tree_event_t {
    INVALID  = 0,
    ON_ENTRY = 0b01,
    ON_EXIT  = 0b10,
    ON_LEAF  = 0b11,
  };
  constexpr bool is_on_entry(tree_event_t);
  constexpr bool is_on_exit(tree_event_t);

  struct error_tree_t {
    struct node_t : public tree_node_t {
      any_vector_index_t memento_buffer_offset;
      any_vector_index_t memento_buffer_offset_end;
      any_vector_index_t memento_buffer_begin;
    };
    array_t<node_t> nodes;

    index_t children_begin(index_t node_index) const;
  };
}

// IMPLEMENTATION

namespace silva {
  inline index_t error_tree_t::children_begin(const index_t node_index) const
  {
    return node_index + 1 - nodes[node_index].subtree_size;
  }

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
