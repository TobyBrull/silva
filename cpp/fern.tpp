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
  "test" : "Hello"
  42
  []
  [
    1
    "two" : 2
    3
  ]
])";
  const auto tt            = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", fern_text)));
  const auto pt_1          = share(SILVA_EXPECT_REQUIRE(fern_parse(tt)));
  const auto pt_2          = SILVA_EXPECT_REQUIRE(fpr->apply(tt));
  CHECK(pt_1->nodes == pt_2->nodes);
  const fern_t fern = SILVA_EXPECT_REQUIRE(fern_create(pt_1.get()));
  CHECK(fern_to_string(pt_1.get()) == fern_text);
  CHECK(fern.to_string() == fern_text);
  CHECK(fern.to_graphviz() == fern_to_graphviz(pt_1.get()));

  const string_view_t expected_parse_tree_str = R"(
[0]`Fern`0                                        [ none true ...
  [0]`LabeledItem`0                               none
    [0]`Item`1                                    none
  [1]`LabeledItem`0                               true
    [0]`Item`1                                    true
  [2]`LabeledItem`0                               "test" : "Hello"
    [0]`Label`0                                   "test"
    [1]`Item`1                                    "Hello"
  [3]`LabeledItem`0                               42
    [0]`Item`1                                    42
  [4]`LabeledItem`0                               [ ]
    [0]`Item`0                                    [ ]
      [0]`Fern`0                                  [ ]
  [5]`LabeledItem`0                               [ 1 "two" ...
    [0]`Item`0                                    [ 1 "two" ...
      [0]`Fern`0                                  [ 1 "two" ...
        [0]`LabeledItem`0                         1
          [0]`Item`1                              1
        [1]`LabeledItem`0                         "two" : 2
          [0]`Label`0                             "two"
          [1]`Item`1                              2
        [2]`LabeledItem`0                         3
          [0]`Item`1                              3
)";

  const string_t result_str = SILVA_EXPECT_REQUIRE(parse_tree_to_string(*pt_1));
  CHECK(result_str == expected_parse_tree_str.substr(1));

  const string_view_t expected_parse_tree_str_graphviz = R"(
digraph parse_tree {
  "/" [label="[0]`Fern`0\n["]
  "/" -> "/0/"
  "/0/" [label="[0]`LabeledItem`0\nnone"]
  "/0/" -> "/0/0/"
  "/0/0/" [label="[0]`Item`1\nnone"]
  "/" -> "/1/"
  "/1/" [label="[1]`LabeledItem`0\ntrue"]
  "/1/" -> "/1/0/"
  "/1/0/" [label="[0]`Item`1\ntrue"]
  "/" -> "/2/"
  "/2/" [label="[2]`LabeledItem`0\n\"test\""]
  "/2/" -> "/2/0/"
  "/2/0/" [label="[0]`Label`0\n\"test\""]
  "/2/" -> "/2/1/"
  "/2/1/" [label="[1]`Item`1\n\"Hello\""]
  "/" -> "/3/"
  "/3/" [label="[3]`LabeledItem`0\n42"]
  "/3/" -> "/3/0/"
  "/3/0/" [label="[0]`Item`1\n42"]
  "/" -> "/4/"
  "/4/" [label="[4]`LabeledItem`0\n["]
  "/4/" -> "/4/0/"
  "/4/0/" [label="[0]`Item`0\n["]
  "/4/0/" -> "/4/0/0/"
  "/4/0/0/" [label="[0]`Fern`0\n["]
  "/" -> "/5/"
  "/5/" [label="[5]`LabeledItem`0\n["]
  "/5/" -> "/5/0/"
  "/5/0/" [label="[0]`Item`0\n["]
  "/5/0/" -> "/5/0/0/"
  "/5/0/0/" [label="[0]`Fern`0\n["]
  "/5/0/0/" -> "/5/0/0/0/"
  "/5/0/0/0/" [label="[0]`LabeledItem`0\n1"]
  "/5/0/0/0/" -> "/5/0/0/0/0/"
  "/5/0/0/0/0/" [label="[0]`Item`1\n1"]
  "/5/0/0/" -> "/5/0/0/1/"
  "/5/0/0/1/" [label="[1]`LabeledItem`0\n\"two\""]
  "/5/0/0/1/" -> "/5/0/0/1/0/"
  "/5/0/0/1/0/" [label="[0]`Label`0\n\"two\""]
  "/5/0/0/1/" -> "/5/0/0/1/1/"
  "/5/0/0/1/1/" [label="[1]`Item`1\n2"]
  "/5/0/0/" -> "/5/0/0/2/"
  "/5/0/0/2/" [label="[2]`LabeledItem`0\n3"]
  "/5/0/0/2/" -> "/5/0/0/2/0/"
  "/5/0/0/2/0/" [label="[0]`Item`1\n3"]
})";
  const string_t result_graphviz = SILVA_EXPECT_REQUIRE(parse_tree_to_graphviz(*pt_1));
  CHECK(result_graphviz == expected_parse_tree_str_graphviz.substr(1));
}
