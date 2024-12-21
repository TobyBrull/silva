#pragma once

#include "assert.hpp"
#include "context.hpp"
#include "error_tree.hpp"
#include "string_or_view.hpp"

namespace silva {

  // For each function that has an "expected_t" return-value, one should always use MINOR errors for
  // the least significant type of error that the function may generate. This should be done even if
  // this type of error is considered of higher level in all other current contexts. The error-level
  // should then be mapped as required, e.g., via "SILVA_EXPECT_FWD(..., MAJOR)".

  enum class error_level_t : uint8_t {
    NONE = 0,

    MINOR  = 1,
    MAJOR  = 2,
    FATAL  = 3,
    ASSERT = 4,
  };
  constexpr bool error_level_is_primary(error_level_t);

  struct error_context_t : public context_t<error_context_t> {
    constexpr static bool context_use_default = true;
    constexpr static bool context_mutable_get = true;

    error_tree_t<string_or_view_t> tree;

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

    string_view_t message() const;

    string_t to_string() const;
  };

  template<typename... Errors>
    requires(std::same_as<Errors, error_t> && ...)
  error_t make_error(error_level_t, string_or_view_t, Errors... child_errors);

  error_t make_error(error_level_t, string_or_view_t, span_t<error_t> child_errors);
}

// IMPLEMENTATION

namespace silva {
  constexpr bool error_level_is_primary(const error_level_t error_level)
  {
    switch (error_level) {
      case error_level_t::NONE:
        return false;

      case error_level_t::MINOR:
      case error_level_t::MAJOR:
      case error_level_t::FATAL:
      case error_level_t::ASSERT:
        return true;
    }
  }

  namespace impl {
    struct error_nursery_t {
      error_context_t* context  = nullptr;
      index_t node_index        = 0;
      index_t num_children      = 0;
      index_t children_begin    = 0;
      error_level_t error_level = error_level_t::NONE;
      string_or_view_t message;

      error_nursery_t(const error_level_t error_level, string_or_view_t message)
        : context(error_context_t::get())
        , node_index(context->tree.nodes.size())
        , children_begin(context->tree.nodes.size())
        , error_level(error_level)
        , message(std::move(message))
      {
      }

      void add_child_error(error_t child_error)
      {
        SILVA_ASSERT(child_error.node_index + 1 == children_begin);
        children_begin = context->tree.nodes[child_error.node_index].children_begin;
        num_children += 1;
        child_error.release();
      }

      error_t finish() &&
      {
        SILVA_ASSERT(context->tree.nodes.size() == node_index);
        context->tree.nodes.push_back(error_tree_t<string_or_view_t>::node_t{
            .num_children   = num_children,
            .children_begin = children_begin,
            .message        = std::move(message),
        });
        return error_t(context, node_index, error_level);
      }
    };
  }

  template<typename... Errors>
    requires(std::same_as<Errors, error_t> && ...)
  error_t
  make_error(const error_level_t error_level, string_or_view_t message, Errors... child_errors)
  {
    impl::error_nursery_t nursery(error_level, std::move(message));
    if constexpr (sizeof...(Errors) > 0) {
      ((nursery.add_child_error(std::move(child_errors)), std::ignore) = ...);
    }
    return std::move(nursery).finish();
  }
}
