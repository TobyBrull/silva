#pragma once

#include "any_vector.hpp"
#include "context.hpp"
#include "error_level.hpp"
#include "error_tree.hpp"

namespace silva {

  struct error_context_t : public context_t<error_context_t> {
    constexpr static bool context_use_default = true;
    constexpr static bool context_mutable_get = true;

    error_tree_t tree;
    to_string_any_vector_t any_vector;

    ~error_context_t();
  };
  using error_context_ptr_t = ptr_t<error_context_t>;

  struct error_t : public sprite_t {
    error_context_ptr_t context;
    index_t node_index  = 0;
    error_level_t level = error_level_t::NO_ERROR;

    ~error_t();
    error_t() = default;
    error_t(error_context_ptr_t, index_t, error_level_t);

    error_t(error_t&& other);
    error_t& operator=(error_t&& other);

    void swap(error_t& other);

    void clear();

    bool is_empty() const;

    void release();

    template<typename... MementoArgs>
    void replace_message(MementoArgs&&...);

    string_or_view_t to_string_plain() const;
    string_or_view_t to_string_structured() const;

    friend void stream_out_impl(stream_t*, const error_t&);

    // Rewrite the error to resolve all pointers/references.
    void materialize();
  };

  struct error_nursery_t {
    error_context_ptr_t context;
    index_t num_children = 0;
    optional_t<index_t> last_node_index;
    optional_t<index_t> children_begin;
    optional_t<any_vector_index_t> memento_buffer_begin;

    error_nursery_t();
    ~error_nursery_t();

    void release();

    void add_child_error(error_t child_error);

    template<typename... MementoArgs>
    error_t finish(error_level_t, MementoArgs&&...) &&;

    error_t finish_single_child_as_is(error_level_t) &&;

    template<typename... MementoArgs>
    error_t finish_short(error_level_t, MementoArgs&&...) &&;
  };

  template<typename... MementoArgs>
  error_t make_error(error_level_t, span_t<error_t> child_errors, MementoArgs&&...);
}

// IMPLEMENTATION

namespace silva {

  template<typename... MementoArgs>
  void error_t::replace_message(MementoArgs&&... memento_args)
  {
    SILVA_ASSERT(!context.is_nullptr() && node_index + 1 == context->tree.nodes.size());
    error_tree_t::node_t& node = context->tree.nodes[node_index];
    context->any_vector.resize_down_to(node.memento_buffer_offset);
    (context->any_vector.push_back(std::forward<MementoArgs>(memento_args)), ...);
    node.memento_buffer_offset_end = context->any_vector.next_index();
  }

  template<typename... MementoArgs>
  error_t make_error(const error_level_t error_level,
                     span_t<error_t> child_errors,
                     MementoArgs&&... memento_args)
  {
    error_nursery_t nursery;
    for (error_t& error: child_errors) {
      nursery.add_child_error(std::move(error));
    }
    return std::move(nursery).finish(error_level, std::forward<MementoArgs>(memento_args)...);
  }

  template<typename... MementoArgs>
  error_t error_nursery_t::finish(const error_level_t error_level, MementoArgs&&... memento_args) &&
  {
    const index_t new_node_index = context->tree.nodes.size();
    SILVA_ASSERT(!last_node_index || *last_node_index + 1 == new_node_index);
    const auto mbo = context->any_vector.next_index();
    (context->any_vector.push_back(std::forward<MementoArgs>(memento_args)), ...);
    context->tree.nodes.push_back(error_tree_t::node_t{
        .num_children              = num_children,
        .children_begin            = children_begin.value_or(new_node_index),
        .memento_buffer_offset     = mbo,
        .memento_buffer_offset_end = context->any_vector.next_index(),
        .memento_buffer_begin      = memento_buffer_begin.value_or(mbo),
    });
    error_t retval(context, new_node_index, error_level);
    release();
    return retval;
  }

  template<typename... MementoArgs>
  error_t error_nursery_t::finish_short(const error_level_t error_level,
                                        MementoArgs&&... memento_args) &&
  {
    if (num_children == 1) {
      return std::move(*this).finish_single_child_as_is(error_level);
    }
    else {
      return std::move(*this).finish(error_level, std::forward<MementoArgs>(memento_args)...);
    }
  }
}
