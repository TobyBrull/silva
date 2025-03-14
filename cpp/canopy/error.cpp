#include "error.hpp"

#include "assert.hpp"

#include <algorithm>
#include <utility>

namespace silva {

  error_context_t::~error_context_t()
  {
    SILVA_ASSERT(tree.nodes.empty());
    SILVA_ASSERT(memento_buffer.buffer.empty());
  }

  namespace impl {
    void error_context_to_string(const error_context_t* error_context,
                                 string_t& retval,
                                 const index_t node_index,
                                 const index_t indent)
    {
      error_context->tree.visit_children_reversed(
          [&](const index_t child_node_index, const index_t child_index) {
            error_context_to_string(error_context, retval, child_node_index, indent + 2);
          },
          node_index);

      const memento_buffer_offset_t mbo =
          error_context->tree.nodes[node_index].memento_buffer_offset;
      const string_or_view_t message =
          error_context->memento_buffer.at_offset(mbo).to_string_or_view();
      retval += fmt::format("{:{}}{}\n", "", indent, message.get_view());
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

  error_t::error_t(error_context_ptr_t context,
                   const index_t node_index,
                   const error_level_t error_level)
    : context(context), node_index(node_index), level(error_level)
  {
  }

  error_t::error_t(error_t&& other)
    : context(std::move(other.context))
    , node_index(std::exchange(other.node_index, 0))
    , level(std::exchange(other.level, error_level_t::NO_ERROR))
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
    if (!context.is_nullptr()) {
      SILVA_ASSERT(context->tree.nodes.size() == node_index + 1);
      const auto& node       = context->tree.nodes[node_index];
      const index_t new_size = node.children_begin;
      context->memento_buffer.resize_offset(node.memento_buffer_begin);
      context->tree.nodes.resize(new_size);
      context.clear();
      node_index = 0;
      level      = error_level_t::NO_ERROR;
    }
  }

  void error_t::release()
  {
    if (!context.is_nullptr()) {
      context.clear();
      node_index = 0;
      level      = error_level_t::NO_ERROR;
    }
  }

  string_or_view_t error_t::message() const
  {
    const memento_buffer_offset_t mbo = context->tree.nodes[node_index].memento_buffer_offset;
    return context->memento_buffer.at_offset(mbo).to_string_or_view();
  }

  string_t error_t::to_string() const
  {
    return context->to_string(node_index);
  }

  void error_t::materialize()
  {
    memento_buffer_t& memento_buffer = context->memento_buffer;
    memento_buffer_t new_memento_buffer;
    hashmap_t<memento_buffer_offset_t, memento_buffer_offset_t> offset_mapping;
    memento_buffer.for_each_memento(
        [&](const memento_buffer_offset_t offset, const memento_t& memento) {
          offset_mapping[offset] = new_memento_buffer.append_memento_materialized(memento);
        });
    const auto& map_offset = [&offset_mapping](memento_buffer_offset_t& offset) {
      const auto it = offset_mapping.find(offset);
      SILVA_ASSERT(it != offset_mapping.end());
      offset = it->second;
    };
    for (auto& node: context->tree.nodes) {
      map_offset(node.memento_buffer_begin);
      map_offset(node.memento_buffer_offset);
    }
    memento_buffer = std::move(new_memento_buffer);
  }

  error_nursery_t::error_nursery_t() : context(error_context_t::get()) {}

  error_nursery_t::~error_nursery_t()
  {
    if (num_children > 0) {
      error_t temp_error = std::move(*this).finish(error_level_t::NO_ERROR);
      // temp_error.~error_t();
    }
  }

  void error_nursery_t::add_child_error(error_t child_error)
  {
    const auto& child_node = context->tree.nodes[child_error.node_index];
    if (num_children == 0) {
      last_node_index      = child_error.node_index;
      children_begin       = child_node.children_begin;
      memento_buffer_begin = child_node.memento_buffer_begin;
    }
    else {
      SILVA_ASSERT(last_node_index && *last_node_index + 1 == child_node.children_begin);
      last_node_index = child_error.node_index;
    }
    num_children += 1;
    child_error.release();
  }

  void error_nursery_t::release()
  {
    num_children = 0;
    last_node_index.reset();
    children_begin.reset();
    memento_buffer_begin.reset();
  }

  error_t error_nursery_t::finish_single_child_as_is(const error_level_t error_level) &&
  {
    SILVA_ASSERT(num_children == 1 && last_node_index.has_value());
    error_t retval(context, last_node_index.value(), error_level);
    release();
    return retval;
  }
}
