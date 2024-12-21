#include "error.hpp"

#include "assert.hpp"

#include <algorithm>
#include <utility>

namespace silva {

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

  string_view_t error_t::message() const
  {
    return context->nodes[node_index].message.get_view();
  }

  void error_t::clear()
  {
    if (context != nullptr) {
      SILVA_ASSERT(context->nodes.size() == node_index + 1);
      const auto& node       = context->nodes[node_index];
      const index_t new_size = node.children_begin;
      context->nodes.resize(new_size);
      context    = nullptr;
      node_index = 0;
      level      = error_level_t::NONE;
    }
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
