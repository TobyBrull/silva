#include "parse_tree.hpp"

#include "rfl/json/write.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

using event_t = parse_tree_event_t;
struct result_t {
  size_t stack_size  = 0;
  index_t node_index = 0;
  event_t event      = event_t::INVALID;

  friend auto operator<=>(const result_t&, const result_t&) = default;
  friend std::ostream& operator<<(std::ostream& os, const result_t& self)
  {
    return os << rfl::json::write(self) << '\n';
  }
};

TEST_CASE("parse_tree", "[parse_tree_t]")
{
  const source_code_t source_code{
      .filename = "some.fern",
      .text     = R"([ 1.1; 2.2; some_label: 3.3; [ 1.0; 3.0; ]; ])",
  };
  const tokenization_t tokens = tokenize(&source_code);

  parse_tree_t pt;
  using n_t = parse_tree_t::node_t;
  pt.nodes.push_back(n_t{.rule_index = 0, .token_index = 0, .num_children = 4, .children_end = 15});
  pt.nodes.push_back(n_t{.rule_index = 1, .token_index = 1, .num_children = 1, .children_end = 3});
  pt.nodes.push_back(n_t{.rule_index = 3, .token_index = 1, .num_children = 0, .children_end = 3});
  pt.nodes.push_back(n_t{.rule_index = 1, .token_index = 3, .num_children = 1, .children_end = 5});
  pt.nodes.push_back(n_t{.rule_index = 3, .token_index = 3, .num_children = 0, .children_end = 5});
  pt.nodes.push_back(n_t{.rule_index = 1, .token_index = 5, .num_children = 2, .children_end = 8});
  pt.nodes.push_back(n_t{.rule_index = 2, .token_index = 5, .num_children = 0, .children_end = 7});
  pt.nodes.push_back(n_t{.rule_index = 3, .token_index = 7, .num_children = 0, .children_end = 8});
  pt.nodes.push_back(n_t{.rule_index = 1, .token_index = 9, .num_children = 1, .children_end = 15});
  pt.nodes.push_back(n_t{.rule_index = 3, .token_index = 9, .num_children = 1, .children_end = 15});
  pt.nodes.push_back(n_t{.rule_index = 0, .token_index = 9, .num_children = 2, .children_end = 15});
  pt.nodes.push_back(
      n_t{.rule_index = 1, .token_index = 10, .num_children = 1, .children_end = 13});
  pt.nodes.push_back(
      n_t{.rule_index = 3, .token_index = 10, .num_children = 0, .children_end = 13});
  pt.nodes.push_back(
      n_t{.rule_index = 1, .token_index = 12, .num_children = 1, .children_end = 15});
  pt.nodes.push_back(
      n_t{.rule_index = 3, .token_index = 12, .num_children = 0, .children_end = 15});

  std::vector<result_t> result;
  REQUIRE(pt.visit_subtree(
      [&](const std::span<const parse_tree_visit_t> stack,
          const event_t event) -> expected_t<void> {
        result.push_back(result_t{
            .stack_size = stack.size(),
            .node_index = stack.back().node_index,
            .event      = event,
        });
        return {};
      }));
  CHECK(
      result ==
      std::vector<result_t>{{
          result_t{.stack_size = 1, .node_index = 0, .event = event_t::ON_ENTRY},

          result_t{.stack_size = 2, .node_index = 1, .event = event_t::ON_ENTRY},
          result_t{.stack_size = 3, .node_index = 2, .event = event_t::ON_LEAF},
          result_t{.stack_size = 2, .node_index = 1, .event = event_t::ON_EXIT},

          result_t{.stack_size = 2, .node_index = 3, .event = event_t::ON_ENTRY},
          result_t{.stack_size = 3, .node_index = 4, .event = event_t::ON_LEAF},
          result_t{.stack_size = 2, .node_index = 3, .event = event_t::ON_EXIT},

          result_t{.stack_size = 2, .node_index = 5, .event = event_t::ON_ENTRY},
          result_t{.stack_size = 3, .node_index = 6, .event = event_t::ON_LEAF},
          result_t{.stack_size = 3, .node_index = 7, .event = event_t::ON_LEAF},
          result_t{.stack_size = 2, .node_index = 5, .event = event_t::ON_EXIT},

          result_t{.stack_size = 2, .node_index = 8, .event = event_t::ON_ENTRY},
          result_t{.stack_size = 3, .node_index = 9, .event = event_t::ON_ENTRY},
          result_t{.stack_size = 4, .node_index = 10, .event = event_t::ON_ENTRY},

          result_t{.stack_size = 5, .node_index = 11, .event = event_t::ON_ENTRY},
          result_t{.stack_size = 6, .node_index = 12, .event = event_t::ON_LEAF},
          result_t{.stack_size = 5, .node_index = 11, .event = event_t::ON_EXIT},

          result_t{.stack_size = 5, .node_index = 13, .event = event_t::ON_ENTRY},
          result_t{.stack_size = 6, .node_index = 14, .event = event_t::ON_LEAF},
          result_t{.stack_size = 5, .node_index = 13, .event = event_t::ON_EXIT},

          result_t{.stack_size = 4, .node_index = 10, .event = event_t::ON_EXIT},
          result_t{.stack_size = 3, .node_index = 9, .event = event_t::ON_EXIT},
          result_t{.stack_size = 2, .node_index = 8, .event = event_t::ON_EXIT},

          result_t{.stack_size = 1, .node_index = 0, .event = event_t::ON_EXIT},
      }});
}
