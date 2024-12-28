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
      context->memento_buffer.resize_offset(node.memento_buffer_begin);
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

  error_nursery_t::error_nursery_t()
    : context(error_context_t::get())
    , node_index(context->tree.nodes.size())
    , children_begin(context->tree.nodes.size())
  {
  }

  void error_nursery_t::add_child_error(error_t child_error)
  {
    SILVA_ASSERT(child_error.node_index + 1 == children_begin);
    const auto& child_node = context->tree.nodes[child_error.node_index];
    children_begin         = child_node.children_begin;
    num_children += 1;
    memento_buffer_begin = std::min(memento_buffer_begin, child_node.memento_buffer_begin);
    child_error.release();
  }
}
