#include "parse_tree.hpp"

#include "rfl/json/write.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;
using enum tree_event_t;

struct result_t {
  size_t stack_size  = 0;
  index_t node_index = 0;
  tree_event_t event = INVALID;

  friend auto operator<=>(const result_t&, const result_t&) = default;
  friend std::ostream& operator<<(std::ostream& os, const result_t& self)
  {
    return os << rfl::json::write(self) << '\n';
  }
};

TEST_CASE("parse_tree", "[parse_tree_t]")
{
  parse_tree_t tree;
  using node_t = parse_tree_t::node_t;
  tree.nodes.push_back(node_t{.num_children = 4, .children_end = 15}); // [0]
  tree.nodes.push_back(node_t{.num_children = 1, .children_end = 3});  // [1]
  tree.nodes.push_back(node_t{.num_children = 0, .children_end = 3});  // [2]
  tree.nodes.push_back(node_t{.num_children = 1, .children_end = 5});  // [3]
  tree.nodes.push_back(node_t{.num_children = 0, .children_end = 5});  // [4]
  tree.nodes.push_back(node_t{.num_children = 2, .children_end = 8});  // [5]
  tree.nodes.push_back(node_t{.num_children = 0, .children_end = 7});  // [6]
  tree.nodes.push_back(node_t{.num_children = 0, .children_end = 8});  // [7]
  tree.nodes.push_back(node_t{.num_children = 1, .children_end = 15}); // [8]
  tree.nodes.push_back(node_t{.num_children = 1, .children_end = 15}); // [9]
  tree.nodes.push_back(node_t{.num_children = 2, .children_end = 15}); // [10]
  tree.nodes.push_back(node_t{.num_children = 1, .children_end = 13}); // [11]
  tree.nodes.push_back(node_t{.num_children = 0, .children_end = 13}); // [12]
  tree.nodes.push_back(node_t{.num_children = 1, .children_end = 15}); // [13]
  tree.nodes.push_back(node_t{.num_children = 0, .children_end = 15}); // [14]

  vector_t<result_t> result;
  REQUIRE(tree.visit_subtree(
      [&](const span_t<const tree_branch_t> path, const tree_event_t event) -> expected_t<bool> {
        result.push_back(result_t{
            .stack_size = path.size(),
            .node_index = path.back().node_index,
            .event      = event,
        });
        return true;
      }));
  CHECK(result ==
        vector_t<result_t>{{
            result_t{.stack_size = 1, .node_index = 0, .event = ON_ENTRY},

            result_t{.stack_size = 2, .node_index = 1, .event = ON_ENTRY},
            result_t{.stack_size = 3, .node_index = 2, .event = ON_LEAF},
            result_t{.stack_size = 2, .node_index = 1, .event = ON_EXIT},

            result_t{.stack_size = 2, .node_index = 3, .event = ON_ENTRY},
            result_t{.stack_size = 3, .node_index = 4, .event = ON_LEAF},
            result_t{.stack_size = 2, .node_index = 3, .event = ON_EXIT},

            result_t{.stack_size = 2, .node_index = 5, .event = ON_ENTRY},
            result_t{.stack_size = 3, .node_index = 6, .event = ON_LEAF},
            result_t{.stack_size = 3, .node_index = 7, .event = ON_LEAF},
            result_t{.stack_size = 2, .node_index = 5, .event = ON_EXIT},

            result_t{.stack_size = 2, .node_index = 8, .event = ON_ENTRY},
            result_t{.stack_size = 3, .node_index = 9, .event = ON_ENTRY},
            result_t{.stack_size = 4, .node_index = 10, .event = ON_ENTRY},

            result_t{.stack_size = 5, .node_index = 11, .event = ON_ENTRY},
            result_t{.stack_size = 6, .node_index = 12, .event = ON_LEAF},
            result_t{.stack_size = 5, .node_index = 11, .event = ON_EXIT},

            result_t{.stack_size = 5, .node_index = 13, .event = ON_ENTRY},
            result_t{.stack_size = 6, .node_index = 14, .event = ON_LEAF},
            result_t{.stack_size = 5, .node_index = 13, .event = ON_EXIT},

            result_t{.stack_size = 4, .node_index = 10, .event = ON_EXIT},
            result_t{.stack_size = 3, .node_index = 9, .event = ON_EXIT},
            result_t{.stack_size = 2, .node_index = 8, .event = ON_EXIT},

            result_t{.stack_size = 1, .node_index = 0, .event = ON_EXIT},
        }});
}
