#include "fern.hpp"

#include "syntax/parse_tree.hpp"
#include "syntax/seed_engine.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("fern", "[fern]")
  {
    const string_view_t fern_text = R"([
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
    token_catalog_t tc;
    const auto tt   = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", fern_text)));
    const auto pt_1 = share(SILVA_EXPECT_REQUIRE(fern_parse(tt)));
    const auto fpr  = fern_seed_engine(tc.ptr());
    const auto pt_2 = SILVA_EXPECT_REQUIRE(fpr->apply(tt, tc.name_id_of("Fern")));
    CHECK(pt_1->nodes == pt_2->nodes);
    const fern_t fern       = SILVA_EXPECT_REQUIRE(fern_create(pt_1.get()));
    const string_t pt_str_1 = SILVA_EXPECT_REQUIRE(fern_to_string(pt_1.get()));
    CHECK(pt_str_1 == fern_text);
    CHECK(fern.to_string() == fern_text);
    const string_t gv_str_1 = SILVA_EXPECT_REQUIRE(fern_to_graphviz(pt_1.get()));
    CHECK(fern.to_graphviz() == gv_str_1);

    const string_view_t expected_parse_tree_str = R"(
[0]_.Fern                                         [ none ... ] ]
  [0]_.Fern.LabeledItem                           none
    [0]_.Fern.Value                               none
  [1]_.Fern.LabeledItem                           true
    [0]_.Fern.Value                               true
  [2]_.Fern.LabeledItem                           'test' : 'Hello'
    [0]_.Fern.Label                               'test'
    [1]_.Fern.Value                               'Hello'
  [3]_.Fern.LabeledItem                           42
    [0]_.Fern.Value                               42
  [4]_.Fern.LabeledItem                           [ ]
    [0]_.Fern                                     [ ]
  [5]_.Fern.LabeledItem                           [ 1 ... 3 ]
    [0]_.Fern                                     [ 1 ... 3 ]
      [0]_.Fern.LabeledItem                       1
        [0]_.Fern.Value                           1
      [1]_.Fern.LabeledItem                       'two' : 2
        [0]_.Fern.Label                           'two'
        [1]_.Fern.Value                           2
      [2]_.Fern.LabeledItem                       3
        [0]_.Fern.Value                           3
)";

    const string_t result_str = SILVA_EXPECT_REQUIRE(pt_1->span().to_string());
    CHECK(result_str == expected_parse_tree_str.substr(1));

    const string_view_t expected_parse_tree_str_graphviz = R"(
digraph parse_tree {
  "/" [label="[0]_.Fern\n["]
  "/" -> "/0/"
  "/0/" [label="[0]_.Fern.LabeledItem\nnone"]
  "/0/" -> "/0/0/"
  "/0/0/" [label="[0]_.Fern.Value\nnone"]
  "/" -> "/1/"
  "/1/" [label="[1]_.Fern.LabeledItem\ntrue"]
  "/1/" -> "/1/0/"
  "/1/0/" [label="[0]_.Fern.Value\ntrue"]
  "/" -> "/2/"
  "/2/" [label="[2]_.Fern.LabeledItem\n'test'"]
  "/2/" -> "/2/0/"
  "/2/0/" [label="[0]_.Fern.Label\n'test'"]
  "/2/" -> "/2/1/"
  "/2/1/" [label="[1]_.Fern.Value\n'Hello'"]
  "/" -> "/3/"
  "/3/" [label="[3]_.Fern.LabeledItem\n42"]
  "/3/" -> "/3/0/"
  "/3/0/" [label="[0]_.Fern.Value\n42"]
  "/" -> "/4/"
  "/4/" [label="[4]_.Fern.LabeledItem\n["]
  "/4/" -> "/4/0/"
  "/4/0/" [label="[0]_.Fern\n["]
  "/" -> "/5/"
  "/5/" [label="[5]_.Fern.LabeledItem\n["]
  "/5/" -> "/5/0/"
  "/5/0/" [label="[0]_.Fern\n["]
  "/5/0/" -> "/5/0/0/"
  "/5/0/0/" [label="[0]_.Fern.LabeledItem\n1"]
  "/5/0/0/" -> "/5/0/0/0/"
  "/5/0/0/0/" [label="[0]_.Fern.Value\n1"]
  "/5/0/" -> "/5/0/1/"
  "/5/0/1/" [label="[1]_.Fern.LabeledItem\n'two'"]
  "/5/0/1/" -> "/5/0/1/0/"
  "/5/0/1/0/" [label="[0]_.Fern.Label\n'two'"]
  "/5/0/1/" -> "/5/0/1/1/"
  "/5/0/1/1/" [label="[1]_.Fern.Value\n2"]
  "/5/0/" -> "/5/0/2/"
  "/5/0/2/" [label="[2]_.Fern.LabeledItem\n3"]
  "/5/0/2/" -> "/5/0/2/0/"
  "/5/0/2/0/" [label="[0]_.Fern.Value\n3"]
})";
    const string_t result_graphviz = SILVA_EXPECT_REQUIRE(pt_1->span().to_graphviz());
    CHECK(result_graphviz == expected_parse_tree_str_graphviz.substr(1));
  }
}
