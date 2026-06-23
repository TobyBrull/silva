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
    const auto se     = standard_seed_interpreter(sf.ptr());
    const auto fp     = SILVA_REQUIRE(fragmentize(sf.ptr(), "", fern_text));
    const auto pt     = SILVA_REQUIRE(se->apply(fp, sf.name_id_of("Fern")));
    const fern_t fern = SILVA_REQUIRE(create(pt.get()));
    CHECK(fern.to_string() == fern_text);

    const string_view_t expected_parse_tree_str = R"(
[  0]   1:1   cat=.literal                                 [
[  1]   2:3   cat=.none                                    none
[  2]   3:3   cat=.boolean                                 true
[  3]   4:3   cat=.string                                  'test'
[  4]   4:10  cat=.literal                                 :
[  5]   4:12  cat=.string                                  'Hello'
[  6]   5:3   cat=.number                                  42
[  7]   6:3   cat=.literal                                 [
[  8]   6:4   cat=.literal                                 ]
[  9]   7:3   cat=.literal                                 [
[ 10]   8:5   cat=.number                                  1
[ 11]   9:5   cat=.string                                  'two'
[ 12]   9:11  cat=.literal                                 :
[ 13]   9:13  cat=.number                                  2
[ 14]  10:5   cat=.number                                  3
[ 15]  11:3   cat=.literal                                 ]
[ 16]  12:1   cat=.literal                                 ]

[0].Fern                                          [ none ... ] ]
  [0].Fern.LabeledItem                            none
    [0].Fern.Value                                none
  [1].Fern.LabeledItem                            true
    [0].Fern.Value                                true
  [2].Fern.LabeledItem                            'test' : 'Hello'
    [0].Fern.Label                                'test'
    [1].Fern.Value                                'Hello'
  [3].Fern.LabeledItem                            42
    [0].Fern.Value                                42
  [4].Fern.LabeledItem                            [ ]
    [0].Fern                                      [ ]
  [5].Fern.LabeledItem                            [ 1 ... 3 ]
    [0].Fern                                      [ 1 ... 3 ]
      [0].Fern.LabeledItem                        1
        [0].Fern.Value                            1
      [1].Fern.LabeledItem                        'two' : 2
        [0].Fern.Label                            'two'
        [1].Fern.Value                            2
      [2].Fern.LabeledItem                        3
        [0].Fern.Value                            3
)";

    const string_t result_str = SILVA_REQUIRE(pt->span().to_string());
    CHECK(result_str == expected_parse_tree_str.substr(1));

    const string_view_t expected_parse_tree_str_graphviz = R"(
digraph parse_tree {
  "/" [label="[0].Fern\n["]
  "/" -> "/0/"
  "/0/" [label="[0].Fern.LabeledItem\nnone"]
  "/0/" -> "/0/0/"
  "/0/0/" [label="[0].Fern.Value\nnone"]
  "/" -> "/1/"
  "/1/" [label="[1].Fern.LabeledItem\ntrue"]
  "/1/" -> "/1/0/"
  "/1/0/" [label="[0].Fern.Value\ntrue"]
  "/" -> "/2/"
  "/2/" [label="[2].Fern.LabeledItem\n'test'"]
  "/2/" -> "/2/0/"
  "/2/0/" [label="[0].Fern.Label\n'test'"]
  "/2/" -> "/2/1/"
  "/2/1/" [label="[1].Fern.Value\n'Hello'"]
  "/" -> "/3/"
  "/3/" [label="[3].Fern.LabeledItem\n42"]
  "/3/" -> "/3/0/"
  "/3/0/" [label="[0].Fern.Value\n42"]
  "/" -> "/4/"
  "/4/" [label="[4].Fern.LabeledItem\n["]
  "/4/" -> "/4/0/"
  "/4/0/" [label="[0].Fern\n["]
  "/" -> "/5/"
  "/5/" [label="[5].Fern.LabeledItem\n["]
  "/5/" -> "/5/0/"
  "/5/0/" [label="[0].Fern\n["]
  "/5/0/" -> "/5/0/0/"
  "/5/0/0/" [label="[0].Fern.LabeledItem\n1"]
  "/5/0/0/" -> "/5/0/0/0/"
  "/5/0/0/0/" [label="[0].Fern.Value\n1"]
  "/5/0/" -> "/5/0/1/"
  "/5/0/1/" [label="[1].Fern.LabeledItem\n'two'"]
  "/5/0/1/" -> "/5/0/1/0/"
  "/5/0/1/0/" [label="[0].Fern.Label\n'two'"]
  "/5/0/1/" -> "/5/0/1/1/"
  "/5/0/1/1/" [label="[1].Fern.Value\n2"]
  "/5/0/" -> "/5/0/2/"
  "/5/0/2/" [label="[2].Fern.LabeledItem\n3"]
  "/5/0/2/" -> "/5/0/2/0/"
  "/5/0/2/0/" [label="[0].Fern.Value\n3"]
})";

    const string_t result_graphviz = SILVA_REQUIRE(pt->span().to_graphviz());
    CHECK(result_graphviz == expected_parse_tree_str_graphviz.substr(1));
  }
}
