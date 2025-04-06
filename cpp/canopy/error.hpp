#pragma once

#include "context.hpp"
#include "error_level.hpp"
#include "error_tree.hpp"
#include "memento.hpp"
#include "string_or_view.hpp"

namespace silva {

  struct error_context_t : public context_t<error_context_t> {
    constexpr static bool context_use_default = true;
    constexpr static bool context_mutable_get = true;

    error_tree_t tree;
    memento_buffer_t memento_buffer;

    ~error_context_t();

    string_t to_string(index_t node_index) const;
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

    string_or_view_t message() const;

    template<typename... MementoArgs>
    void replace_message(MementoArgs&&...);

    string_t to_string() const;

    // Rewrite the error to resolve all pointers/references.
    void materialize();
  };

  struct error_nursery_t {
    error_context_ptr_t context;
    index_t num_children = 0;
    optional_t<index_t> last_node_index;
    optional_t<index_t> children_begin;
    optional_t<index_t> memento_buffer_begin;

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
    context->memento_buffer.resize_offset(node.memento_buffer_offset);
    context->memento_buffer.append_memento(std::forward<MementoArgs>(memento_args)...);
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
    const index_t mbo =
        context->memento_buffer.append_memento(std::forward<MementoArgs>(memento_args)...);
    context->tree.nodes.push_back(error_tree_t::node_t{
        .num_children          = num_children,
        .children_begin        = children_begin.value_or(new_node_index),
        .memento_buffer_offset = mbo,
        .memento_buffer_begin  = memento_buffer_begin.value_or(mbo),
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
