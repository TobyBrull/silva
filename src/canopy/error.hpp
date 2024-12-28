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

  struct error_t : public sprite_t {
    error_context_t* context = nullptr;
    index_t node_index       = 0;
    error_level_t level      = error_level_t::NONE;

    ~error_t();
    error_t() = default;
    error_t(error_context_t*, index_t, error_level_t);

    error_t(error_t&& other);
    error_t& operator=(error_t&& other);

    void swap(error_t& other);

    void clear();

    void release();

    string_or_view_t message() const;

    string_t to_string() const;

    // Rewrite the error to resolve all pointers/references.
    void materialize();
  };

  struct error_nursery_t {
    error_context_t* context     = nullptr;
    index_t node_index           = 0;
    index_t num_children         = 0;
    index_t children_begin       = 0;
    index_t memento_buffer_begin = std::numeric_limits<index_t>::max();

    error_nursery_t();

    void add_child_error(error_t child_error);

    template<typename... MementoArgs>
    error_t finish(error_level_t, MementoArgs&&... memento_args) &&;
  };

  template<typename... MementoArgs>
  error_t make_error(error_level_t, span_t<error_t> child_errors, MementoArgs&&...);
}

// IMPLEMENTATION

namespace silva {
  template<typename... MementoArgs>
  error_t make_error(const error_level_t error_level,
                     span_t<error_t> child_errors,
                     MementoArgs&&... memento_args)
  {
    error_nursery_t nursery;
    for (auto it = child_errors.rbegin(); it != child_errors.rend(); ++it) {
      error_t& error = *it;
      nursery.add_child_error(std::move(error));
    }
    return std::move(nursery).finish(error_level, std::forward<MementoArgs>(memento_args)...);
  }

  template<typename... MementoArgs>
  error_t error_nursery_t::finish(const error_level_t error_level, MementoArgs&&... memento_args) &&
  {
    SILVA_ASSERT(context->tree.nodes.size() == node_index);
    const index_t mbo =
        context->memento_buffer.append_memento(std::forward<MementoArgs>(memento_args)...);
    context->tree.nodes.push_back(error_tree_t::node_t{
        .num_children          = num_children,
        .children_begin        = children_begin,
        .memento_buffer_offset = mbo,
        .memento_buffer_begin  = std::min(mbo, memento_buffer_begin),
    });
    return error_t(context, node_index, error_level);
  }
}
