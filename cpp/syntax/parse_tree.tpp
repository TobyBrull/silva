#include "parse_tree.hpp"

#include "rfl/json/write.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
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
    using node_t = parse_tree_node_t;
    tree.nodes.push_back(node_t{{.num_children = 4, .subtree_size = 15}}); // [0]
    tree.nodes.push_back(node_t{{.num_children = 1, .subtree_size = 2}});  // [1]
    tree.nodes.push_back(node_t{{.num_children = 0, .subtree_size = 1}});  // [2]
    tree.nodes.push_back(node_t{{.num_children = 1, .subtree_size = 2}});  // [3]
    tree.nodes.push_back(node_t{{.num_children = 0, .subtree_size = 1}});  // [4]
    tree.nodes.push_back(node_t{{.num_children = 2, .subtree_size = 3}});  // [5]
    tree.nodes.push_back(node_t{{.num_children = 0, .subtree_size = 1}});  // [6]
    tree.nodes.push_back(node_t{{.num_children = 0, .subtree_size = 1}});  // [7]
    tree.nodes.push_back(node_t{{.num_children = 1, .subtree_size = 7}});  // [8]
    tree.nodes.push_back(node_t{{.num_children = 1, .subtree_size = 6}});  // [9]
    tree.nodes.push_back(node_t{{.num_children = 2, .subtree_size = 5}});  // [10]
    tree.nodes.push_back(node_t{{.num_children = 1, .subtree_size = 2}});  // [11]
    tree.nodes.push_back(node_t{{.num_children = 0, .subtree_size = 1}});  // [12]
    tree.nodes.push_back(node_t{{.num_children = 1, .subtree_size = 2}});  // [13]
    tree.nodes.push_back(node_t{{.num_children = 0, .subtree_size = 1}});  // [14]

    vector_t<result_t> result;
    REQUIRE(tree.span().visit_subtree(
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
}
