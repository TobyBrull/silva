#include "fern.hpp"

#include "syntax/parse_tree.hpp"
#include "syntax/seed_engine.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("fern", "[fern]")
{
  token_context_t tc;
  const string_t fern_text = R"([
  none
  true
  'test' : 'Hello'
  42
  []
  [
    1
    'two' : 2
    3
  ]
])";
  const auto tt            = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", fern_text)));
  const auto pt_1          = share(SILVA_EXPECT_REQUIRE(fern_parse(tt)));
  const auto fpr           = fern_seed_engine(tc.ptr());
  const auto pt_2          = SILVA_EXPECT_REQUIRE(fpr->apply(tt, tc.name_id_of("Fern")));
  CHECK(pt_1->nodes == pt_2->nodes);
  const fern_t fern       = SILVA_EXPECT_REQUIRE(fern_create(pt_1.get()));
  const string_t pt_str_1 = SILVA_EXPECT_REQUIRE(fern_to_string(pt_1.get()));
  CHECK(pt_str_1 == fern_text);
  CHECK(fern.to_string() == fern_text);
  const string_t gv_str_1 = SILVA_EXPECT_REQUIRE(fern_to_graphviz(pt_1.get()));
  CHECK(fern.to_graphviz() == gv_str_1);

  const string_view_t expected_parse_tree_str = R"(
[0]Silva.Fern                                     [ none ... ] ]
  [0]Silva.LabeledItem                            none
    [0]Silva.Value                                none
  [1]Silva.LabeledItem                            true
    [0]Silva.Value                                true
  [2]Silva.LabeledItem                            'test' : 'Hello'
    [0]Silva.Label                                'test'
    [1]Silva.Value                                'Hello'
  [3]Silva.LabeledItem                            42
    [0]Silva.Value                                42
  [4]Silva.LabeledItem                            [ ]
    [0]Silva.Fern                                 [ ]
  [5]Silva.LabeledItem                            [ 1 ... 3 ]
    [0]Silva.Fern                                 [ 1 ... 3 ]
      [0]Silva.LabeledItem                        1
        [0]Silva.Value                            1
      [1]Silva.LabeledItem                        'two' : 2
        [0]Silva.Label                            'two'
        [1]Silva.Value                            2
      [2]Silva.LabeledItem                        3
        [0]Silva.Value                            3
)";

  const string_t result_str = SILVA_EXPECT_REQUIRE(parse_tree_to_string(*pt_1));
  CHECK(result_str == expected_parse_tree_str.substr(1));

  const string_view_t expected_parse_tree_str_graphviz = R"(
digraph parse_tree {
  "/" [label="[0]Silva.Fern\n["]
  "/" -> "/0/"
  "/0/" [label="[0]Silva.LabeledItem\nnone"]
  "/0/" -> "/0/0/"
  "/0/0/" [label="[0]Silva.Value\nnone"]
  "/" -> "/1/"
  "/1/" [label="[1]Silva.LabeledItem\ntrue"]
  "/1/" -> "/1/0/"
  "/1/0/" [label="[0]Silva.Value\ntrue"]
  "/" -> "/2/"
  "/2/" [label="[2]Silva.LabeledItem\n'test'"]
  "/2/" -> "/2/0/"
  "/2/0/" [label="[0]Silva.Label\n'test'"]
  "/2/" -> "/2/1/"
  "/2/1/" [label="[1]Silva.Value\n'Hello'"]
  "/" -> "/3/"
  "/3/" [label="[3]Silva.LabeledItem\n42"]
  "/3/" -> "/3/0/"
  "/3/0/" [label="[0]Silva.Value\n42"]
  "/" -> "/4/"
  "/4/" [label="[4]Silva.LabeledItem\n["]
  "/4/" -> "/4/0/"
  "/4/0/" [label="[0]Silva.Fern\n["]
  "/" -> "/5/"
  "/5/" [label="[5]Silva.LabeledItem\n["]
  "/5/" -> "/5/0/"
  "/5/0/" [label="[0]Silva.Fern\n["]
  "/5/0/" -> "/5/0/0/"
  "/5/0/0/" [label="[0]Silva.LabeledItem\n1"]
  "/5/0/0/" -> "/5/0/0/0/"
  "/5/0/0/0/" [label="[0]Silva.Value\n1"]
  "/5/0/" -> "/5/0/1/"
  "/5/0/1/" [label="[1]Silva.LabeledItem\n'two'"]
  "/5/0/1/" -> "/5/0/1/0/"
  "/5/0/1/0/" [label="[0]Silva.Label\n'two'"]
  "/5/0/1/" -> "/5/0/1/1/"
  "/5/0/1/1/" [label="[1]Silva.Value\n2"]
  "/5/0/" -> "/5/0/2/"
  "/5/0/2/" [label="[2]Silva.LabeledItem\n3"]
  "/5/0/2/" -> "/5/0/2/0/"
  "/5/0/2/0/" [label="[0]Silva.Value\n3"]
})";
  const string_t result_graphviz = SILVA_EXPECT_REQUIRE(parse_tree_to_graphviz(*pt_1));
  CHECK(result_graphviz == expected_parse_tree_str_graphviz.substr(1));
}
