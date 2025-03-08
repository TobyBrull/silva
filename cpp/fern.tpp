#include "fern.hpp"
#include "parse_root.hpp"
#include "parse_tree.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("fern", "[fern]")
{
  token_context_t tc;
  const auto fpr           = fern_parse_root(tc.ptr());
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
  const auto pt_2          = SILVA_EXPECT_REQUIRE(fpr->apply(tt));
  CHECK(pt_1->nodes == pt_2->nodes);
  const fern_t fern       = SILVA_EXPECT_REQUIRE(fern_create(pt_1.get()));
  const string_t pt_str_1 = SILVA_EXPECT_REQUIRE(fern_to_string(pt_1.get()));
  CHECK(pt_str_1 == fern_text);
  CHECK(fern.to_string() == fern_text);
  const string_t gv_str_1 = SILVA_EXPECT_REQUIRE(fern_to_graphviz(pt_1.get()));
  CHECK(fern.to_graphviz() == gv_str_1);

  const string_view_t expected_parse_tree_str = R"(
[0]~Fern                                          [ none true ...
  [0]~LabeledItem                                 none
    [0]~Value                                     none
  [1]~LabeledItem                                 true
    [0]~Value                                     true
  [2]~LabeledItem                                 'test' : 'Hello'
    [0]~Label                                     'test'
    [1]~Value                                     'Hello'
  [3]~LabeledItem                                 42
    [0]~Value                                     42
  [4]~LabeledItem                                 [ ]
    [0]~Fern                                      [ ]
  [5]~LabeledItem                                 [ 1 'two' ...
    [0]~Fern                                      [ 1 'two' ...
      [0]~LabeledItem                             1
        [0]~Value                                 1
      [1]~LabeledItem                             'two' : 2
        [0]~Label                                 'two'
        [1]~Value                                 2
      [2]~LabeledItem                             3
        [0]~Value                                 3
)";

  const string_t result_str = SILVA_EXPECT_REQUIRE(parse_tree_to_string(*pt_1));
  CHECK(result_str == expected_parse_tree_str.substr(1));

  const string_view_t expected_parse_tree_str_graphviz = R"(
digraph parse_tree {
  "/" [label="[0]~Fern\n["]
  "/" -> "/0/"
  "/0/" [label="[0]~LabeledItem\nnone"]
  "/0/" -> "/0/0/"
  "/0/0/" [label="[0]~Value\nnone"]
  "/" -> "/1/"
  "/1/" [label="[1]~LabeledItem\ntrue"]
  "/1/" -> "/1/0/"
  "/1/0/" [label="[0]~Value\ntrue"]
  "/" -> "/2/"
  "/2/" [label="[2]~LabeledItem\n'test'"]
  "/2/" -> "/2/0/"
  "/2/0/" [label="[0]~Label\n'test'"]
  "/2/" -> "/2/1/"
  "/2/1/" [label="[1]~Value\n'Hello'"]
  "/" -> "/3/"
  "/3/" [label="[3]~LabeledItem\n42"]
  "/3/" -> "/3/0/"
  "/3/0/" [label="[0]~Value\n42"]
  "/" -> "/4/"
  "/4/" [label="[4]~LabeledItem\n["]
  "/4/" -> "/4/0/"
  "/4/0/" [label="[0]~Fern\n["]
  "/" -> "/5/"
  "/5/" [label="[5]~LabeledItem\n["]
  "/5/" -> "/5/0/"
  "/5/0/" [label="[0]~Fern\n["]
  "/5/0/" -> "/5/0/0/"
  "/5/0/0/" [label="[0]~LabeledItem\n1"]
  "/5/0/0/" -> "/5/0/0/0/"
  "/5/0/0/0/" [label="[0]~Value\n1"]
  "/5/0/" -> "/5/0/1/"
  "/5/0/1/" [label="[1]~LabeledItem\n'two'"]
  "/5/0/1/" -> "/5/0/1/0/"
  "/5/0/1/0/" [label="[0]~Label\n'two'"]
  "/5/0/1/" -> "/5/0/1/1/"
  "/5/0/1/1/" [label="[1]~Value\n2"]
  "/5/0/" -> "/5/0/2/"
  "/5/0/2/" [label="[2]~LabeledItem\n3"]
  "/5/0/2/" -> "/5/0/2/0/"
  "/5/0/2/0/" [label="[0]~Value\n3"]
})";
  const string_t result_graphviz = SILVA_EXPECT_REQUIRE(parse_tree_to_graphviz(*pt_1));
  CHECK(result_graphviz == expected_parse_tree_str_graphviz.substr(1));
}
