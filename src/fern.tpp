#include "fern.hpp"
#include "parse_root.hpp"
#include "parse_tree.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("fern", "[fern]")
{
  const source_code_t fern_source_code("simple.fern", R"([
  none;
  true;
  "test" : "Hello";
  42;
  [];
  [
    1;
    "two" : 2;
    3;
  ];
])");

  const const_ptr_t<tokenization_t> tokenization =
      to_unique_ptr(SILVA_EXPECT_REQUIRE(tokenize(const_ptr_unowned(&fern_source_code))));

  const parse_tree_t pt_1 = SILVA_EXPECT_REQUIRE(fern_parse(tokenization));
  const parse_tree_t pt_2 = SILVA_EXPECT_REQUIRE(fern_parse_root()->apply(tokenization));
  CHECK(pt_1.nodes == pt_2.nodes);

  const fern_t fern = SILVA_EXPECT_REQUIRE(fern_create(&pt_1));

  CHECK(fern_to_string(&pt_1) == fern_source_code.text);
  CHECK(fern.to_string() == fern_source_code.text);
  CHECK(fern.to_graphviz() == fern_to_graphviz(&pt_1));

  const string_view_t expected_parse_tree_str = R"(
[0]Fern,0                                         [
  [0]LabeledItem,0                                none
    [0]Item,1                                     none
  [1]LabeledItem,0                                true
    [0]Item,1                                     true
  [2]LabeledItem,0                                "test"
    [0]Label,0                                    "test"
    [1]Item,1                                     "Hello"
  [3]LabeledItem,0                                42
    [0]Item,1                                     42
  [4]LabeledItem,0                                [
    [0]Item,0                                     [
      [0]Fern,0                                   [
  [5]LabeledItem,0                                [
    [0]Item,0                                     [
      [0]Fern,0                                   [
        [0]LabeledItem,0                          1
          [0]Item,1                               1
        [1]LabeledItem,0                          "two"
          [0]Label,0                              "two"
          [1]Item,1                               2
        [2]LabeledItem,0                          3
          [0]Item,1                               3
)";
  CHECK(parse_tree_to_string(pt_1) == expected_parse_tree_str.substr(1));

  const string_view_t expected_parse_tree_str_graphviz = R"(
digraph parse_tree {
  "/" [label="[0]Fern,0\n["]
  "/" -> "/0/"
  "/0/" [label="[0]LabeledItem,0\nnone"]
  "/0/" -> "/0/0/"
  "/0/0/" [label="[0]Item,1\nnone"]
  "/" -> "/1/"
  "/1/" [label="[1]LabeledItem,0\ntrue"]
  "/1/" -> "/1/0/"
  "/1/0/" [label="[0]Item,1\ntrue"]
  "/" -> "/2/"
  "/2/" [label="[2]LabeledItem,0\n\"test\""]
  "/2/" -> "/2/0/"
  "/2/0/" [label="[0]Label,0\n\"test\""]
  "/2/" -> "/2/1/"
  "/2/1/" [label="[1]Item,1\n\"Hello\""]
  "/" -> "/3/"
  "/3/" [label="[3]LabeledItem,0\n42"]
  "/3/" -> "/3/0/"
  "/3/0/" [label="[0]Item,1\n42"]
  "/" -> "/4/"
  "/4/" [label="[4]LabeledItem,0\n["]
  "/4/" -> "/4/0/"
  "/4/0/" [label="[0]Item,0\n["]
  "/4/0/" -> "/4/0/0/"
  "/4/0/0/" [label="[0]Fern,0\n["]
  "/" -> "/5/"
  "/5/" [label="[5]LabeledItem,0\n["]
  "/5/" -> "/5/0/"
  "/5/0/" [label="[0]Item,0\n["]
  "/5/0/" -> "/5/0/0/"
  "/5/0/0/" [label="[0]Fern,0\n["]
  "/5/0/0/" -> "/5/0/0/0/"
  "/5/0/0/0/" [label="[0]LabeledItem,0\n1"]
  "/5/0/0/0/" -> "/5/0/0/0/0/"
  "/5/0/0/0/0/" [label="[0]Item,1\n1"]
  "/5/0/0/" -> "/5/0/0/1/"
  "/5/0/0/1/" [label="[1]LabeledItem,0\n\"two\""]
  "/5/0/0/1/" -> "/5/0/0/1/0/"
  "/5/0/0/1/0/" [label="[0]Label,0\n\"two\""]
  "/5/0/0/1/" -> "/5/0/0/1/1/"
  "/5/0/0/1/1/" [label="[1]Item,1\n2"]
  "/5/0/0/" -> "/5/0/0/2/"
  "/5/0/0/2/" [label="[2]LabeledItem,0\n3"]
  "/5/0/0/2/" -> "/5/0/0/2/0/"
  "/5/0/0/2/0/" [label="[0]Item,1\n3"]
})";
  CHECK(parse_tree_to_graphviz(pt_1) == expected_parse_tree_str_graphviz.substr(1));
}
