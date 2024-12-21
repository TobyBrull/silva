#include "error.hpp"

#include "assert.hpp"

#include <algorithm>
#include <utility>

namespace silva {

  error_context_t::~error_context_t()
  {
    SILVA_ASSERT(tree.nodes.empty());
  }

  namespace impl {
    void error_context_to_string(const error_context_t* error_context,
                                 string_t& retval,
                                 const index_t node_index,
                                 const index_t indent)
    {
      retval += fmt::format("{:{}}{}\n",
                            "",
                            indent,
                            error_context->tree.nodes[node_index].message.get_view());
      auto& error_tree           = error_context->tree;
      const index_t num_children = error_tree.nodes[node_index].num_children;
      index_t curr_node_index    = node_index;
      for (index_t i = 0; i < num_children; ++i) {
        curr_node_index -= 1;
        error_context_to_string(error_context, retval, curr_node_index, indent + 2);
        curr_node_index = error_tree.nodes[curr_node_index].children_begin;
      }
    }
  }

  string_t error_context_t::to_string(index_t node_index) const
  {
    string_t retval;
    impl::error_context_to_string(this, retval, node_index, 0);
    return retval;
  }

  error_t::~error_t()
  {
    clear();
  }

  error_t::error_t(error_context_t* context,
                   const index_t node_index,
                   const error_level_t error_level)
    : context(context), node_index(node_index), level(error_level)
  {
  }

  error_t::error_t(error_t&& other)
    : context(std::exchange(other.context, nullptr))
    , node_index(std::exchange(other.node_index, 0))
    , level(std::exchange(other.level, error_level_t::NONE))
  {
  }

  error_t& error_t::operator=(error_t&& other)
  {
    if (this != &other) {
      error_t temp(std::move(other));
      this->swap(temp);
    }
    return *this;
  }

  void error_t::swap(error_t& other)
  {
    std::swap(context, other.context);
    std::swap(node_index, other.node_index);
    std::swap(level, other.level);
  }

  void error_t::clear()
  {
    if (context != nullptr) {
      SILVA_ASSERT(context->tree.nodes.size() == node_index + 1);
      const auto& node       = context->tree.nodes[node_index];
      const index_t new_size = node.children_begin;
      context->tree.nodes.resize(new_size);
      context    = nullptr;
      node_index = 0;
      level      = error_level_t::NONE;
    }
  }

  void error_t::release()
  {
    if (context != nullptr) {
      context    = nullptr;
      node_index = 0;
      level      = error_level_t::NONE;
    }
  }

  string_view_t error_t::message() const
  {
    return context->tree.nodes[node_index].message.get_view();
  }

  string_t error_t::to_string() const
  {
    return context->to_string(node_index);
  }

  error_t make_error(const error_level_t error_level,
                     string_or_view_t message,
                     span_t<error_t> child_errors)
  {
    impl::error_nursery_t nursery(error_level, std::move(message));
    for (auto it = child_errors.rbegin(); it != child_errors.rend(); ++it) {
      error_t& error = *it;
      nursery.add_child_error(std::move(error));
    }
    return std::move(nursery).finish();
  }
}
