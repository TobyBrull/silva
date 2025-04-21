#include "error_tree.hpp"

#include "format.hpp"

namespace silva {
  string_t error_tree_t::node_t::to_string(const to_string_any_vector_t& av) const
  {
    vector_t<string_t> args;
    const auto end = av.index_iter_at(memento_buffer_offset_end);
    for (auto it = av.index_iter_at(memento_buffer_offset); it != end; ++it) {
      args.push_back(av.apply(*it, silva::to_string).as_string());
    }
    string_t message = format_vector(args);
    return message;
  }
}
