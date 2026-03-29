#include "fern.hpp"

#include "syntax/syntax.hpp"

#include <catch2/catch_all.hpp>

namespace silva::fern::test {
  TEST_CASE("fern", "[fern]")
  {
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
]
)";

    syntax_farm_t sf;
    const auto se = standard_seed_interpreter(sf.ptr());
    const auto tt =
        SILVA_REQUIRE(se->tokenizer_farm.apply_text("", fern_text, sf.token_id("Fern")));
    const auto pt     = SILVA_REQUIRE(se->apply(tt, sf.name_id_of("Fern")));
    const fern_t fern = SILVA_REQUIRE(create(pt.get()));
    CHECK(fern.to_string() == fern_text);

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

    const string_t result_str = SILVA_REQUIRE(pt->span().to_string());
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

    const string_t result_graphviz = SILVA_REQUIRE(pt->span().to_graphviz());
    CHECK(result_graphviz == expected_parse_tree_str_graphviz.substr(1));
  }
}
