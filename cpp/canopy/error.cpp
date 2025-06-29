#include "error.hpp"

#include "assert.hpp"
#include "format.hpp"

#include <algorithm>
#include <utility>

namespace silva {
  string_t to_string(const error_tree_t::node_t& node,
                     const any_vector_t<pretty_write_string_t, move_ctor_t, dtor_t>& av)
  {
    vector_t<string_t> args;
    const auto end = av.index_iter_at(node.memento_buffer_offset_end);
    for (auto it = av.index_iter_at(node.memento_buffer_offset); it != end; ++it) {
      args.push_back(av.apply(*it, silva::pretty_write_string));
    }
    string_t message = format_vector(args);
    return message;
  }

  error_context_t::~error_context_t()
  {
    SILVA_ASSERT(tree.nodes.empty());
    SILVA_ASSERT(any_vector.is_empty());
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
      context->any_vector.resize_down_to(node.memento_buffer_begin);
      context->tree.nodes.resize(new_size);
      context.clear();
      node_index = 0;
      level      = error_level_t::NO_ERROR;
    }
  }

  bool error_t::is_empty() const
  {
    return context.is_nullptr();
  }

  void error_t::release()
  {
    if (!context.is_nullptr()) {
      context.clear();
      node_index = 0;
      level      = error_level_t::NO_ERROR;
    }
  }

  namespace impl {
    void to_string_plain(const error_context_t* error_context,
                         string_t& retval,
                         const index_t node_index,
                         const index_t indent)
    {
      error_context->tree.visit_children_reversed(
          [&](const index_t child_node_index, const index_t child_index) {
            to_string_plain(error_context, retval, child_node_index, indent + 2);
          },
          node_index);
      const auto& node       = error_context->tree.nodes[node_index];
      const string_t message = to_string(node, error_context->any_vector);
      retval += fmt::format("{:{}}{}\n", "", indent, message);
    }
  }

  string_or_view_t error_t::to_string_plain() const
  {
    string_t retval;
    impl::to_string_plain(context.get(), retval, node_index, 0);
    return string_or_view_t{std::move(retval)};
  }

  namespace impl {
    const static vector_t<string_view_t> box_chars = {
        "  ", // [0]
        "│ ", // [1]
        "├─", // [2]
        "┌─", // [3]

        "─ ",
        "┐ ",
        "┘ ",
        "┌ ",
        "└ ",
        "├ ",
        "┤ ",
        "┬ ",
        "┴ ",
        "┼ ",
    };

    void to_string_struct_indent(string_t& retval, vector_t<index_t>& box_levels)
    {
      for (const index_t idx: box_levels) {
        retval += box_chars[idx];
      }
    }

    enum class state_t {
      NONE,
      NEST_LAST,
      NEST_OTHER,
    };

    void to_string_struct(const error_context_t* error_context,
                          string_t& retval,
                          vector_t<index_t>& box_levels,
                          const index_t node_index,
                          const state_t state)
    {
      const index_t num_children = error_context->tree.nodes[node_index].num_children;
      if (num_children <= 1) {
        error_context->tree.visit_children_reversed(
            [&](const index_t child_node_index, const index_t child_index) {
              to_string_struct(error_context, retval, box_levels, child_node_index, state_t::NONE);
            },
            node_index);
      }
      else {
        error_context->tree.visit_children_reversed(
            [&](const index_t child_node_index, const index_t child_index) {
              state_t new_state = state_t::NONE;
              if (child_index + 1 == num_children) {
                box_levels.push_back(0);
                new_state = state_t::NEST_LAST;
              }
              else {
                box_levels.push_back(1);
                new_state = state_t::NEST_OTHER;
              }
              to_string_struct(error_context, retval, box_levels, child_node_index, new_state);
              box_levels.pop_back();
            },
            node_index);
      }
      optional_t<index_t> prev_back;
      if (state != state_t::NONE) {
        prev_back         = box_levels.back();
        box_levels.back() = (state == state_t::NEST_OTHER) ? 2 : 3;
      }
      to_string_struct_indent(retval, box_levels);
      if (prev_back.has_value()) {
        box_levels.back() = prev_back.value();
      }
      const auto& node       = error_context->tree.nodes[node_index];
      const string_t message = to_string(node, error_context->any_vector);
      retval += message + "\n";
    }
  }

  string_or_view_t error_t::to_string_structured() const
  {
    string_t retval;
    vector_t<index_t> box_levels;
    impl::to_string_struct(context.get(), retval, box_levels, node_index, impl::state_t::NONE);
    return string_or_view_t{std::move(retval)};
  }

  void pretty_write_impl(const error_t& self, byte_sink_t* stream)
  {
    stream->write_str(self.to_string_structured().as_string_view());
  }

  void error_t::materialize()
  {
    auto& any_vector = context->any_vector;
    any_vector_t<pretty_write_string_t, move_ctor_t, dtor_t> new_any_vector;
    hash_map_t<any_vector_index_t, any_vector_index_t> offset_mapping;
    {
      for (const auto avi: any_vector.index_range()) {
        string_t x          = any_vector.apply(avi, pretty_write_string);
        offset_mapping[avi] = new_any_vector.push_back(std::move(x));
      }
      offset_mapping[any_vector.next_index()] = new_any_vector.next_index();
    }
    const auto& map_offset = [&offset_mapping](any_vector_index_t& offset) {
      const auto it = offset_mapping.find(offset);
      SILVA_ASSERT(it != offset_mapping.end());
      offset = it->second;
    };
    for (auto& node: context->tree.nodes) {
      map_offset(node.memento_buffer_offset);
      map_offset(node.memento_buffer_offset_end);
      map_offset(node.memento_buffer_begin);
    }
    any_vector = std::move(new_any_vector);
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
