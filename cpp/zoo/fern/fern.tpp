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
[0].Fern                                          [\n  n ...  ]\n]<NEWLINE>
  [0].Fern.LabeledItem                            none\n  
    [0].Fern.value                                none
      [0].none                                    none
  [1].Fern.LabeledItem                            true\n  
    [0].Fern.value                                true
      [0].boolean                                 true
  [2].Fern.LabeledItem                            'test' : 'Hello'\n  
    [0].Fern.label                                'test'
      [0].string                                  'test'
    [1].Fern.value                                'Hello'
      [0].string                                  'Hello'
  [3].Fern.LabeledItem                            42\n  
    [0].Fern.value                                42
      [0].number                                  42
        [0].number.integer                        42
          [0].number.integer.decimal              42
            [0].number.plusMinus                  
            [1].number.unsigned.integer.decimal   42
  [4].Fern.LabeledItem                            []\n  
    [0].Fern                                      []\n  
  [5].Fern.LabeledItem                            [\n    ... \n  ]\n
    [0].Fern                                      [\n    ... \n  ]\n
      [0].Fern.LabeledItem                        1\n    
        [0].Fern.value                            1
          [0].number                              1
            [0].number.integer                    1
              [0].number.integer.decimal          1
                [0].number.plusMinus              
                [1].number.unsigned.integer.decimal 1
      [1].Fern.LabeledItem                        'two' : 2\n    
        [0].Fern.label                            'two'
          [0].string                              'two'
        [1].Fern.value                            2
          [0].number                              2
            [0].number.integer                    2
              [0].number.integer.decimal          2
                [0].number.plusMinus              
                [1].number.unsigned.integer.decimal 2
      [2].Fern.LabeledItem                        3\n  
        [0].Fern.value                            3
          [0].number                              3
            [0].number.integer                    3
              [0].number.integer.decimal          3
                [0].number.plusMinus              
                [1].number.unsigned.integer.decimal 3
)";

    const string_t result_str = SILVA_REQUIRE(pt->span().to_string());
    CHECK(result_str == expected_parse_tree_str.substr(1));

    const string_view_t expected_parse_tree_str_graphviz = R"(
digraph parse_tree {
  "/" [label="[0].Fern\n[\\n  n ...  ]\\n]<NEWLINE>"]
  "/" -> "/0/"
  "/0/" [label="[0].Fern.LabeledItem\nnone\\n  "]
  "/0/" -> "/0/0/"
  "/0/0/" [label="[0].Fern.value\nnone"]
  "/0/0/" -> "/0/0/0/"
  "/0/0/0/" [label="[0].none\nnone"]
  "/" -> "/1/"
  "/1/" [label="[1].Fern.LabeledItem\ntrue\\n  "]
  "/1/" -> "/1/0/"
  "/1/0/" [label="[0].Fern.value\ntrue"]
  "/1/0/" -> "/1/0/0/"
  "/1/0/0/" [label="[0].boolean\ntrue"]
  "/" -> "/2/"
  "/2/" [label="[2].Fern.LabeledItem\n'test' : 'Hello'\\n  "]
  "/2/" -> "/2/0/"
  "/2/0/" [label="[0].Fern.label\n'test'"]
  "/2/0/" -> "/2/0/0/"
  "/2/0/0/" [label="[0].string\n'test'"]
  "/2/" -> "/2/1/"
  "/2/1/" [label="[1].Fern.value\n'Hello'"]
  "/2/1/" -> "/2/1/0/"
  "/2/1/0/" [label="[0].string\n'Hello'"]
  "/" -> "/3/"
  "/3/" [label="[3].Fern.LabeledItem\n42\\n  "]
  "/3/" -> "/3/0/"
  "/3/0/" [label="[0].Fern.value\n42"]
  "/3/0/" -> "/3/0/0/"
  "/3/0/0/" [label="[0].number\n42"]
  "/3/0/0/" -> "/3/0/0/0/"
  "/3/0/0/0/" [label="[0].number.integer\n42"]
  "/3/0/0/0/" -> "/3/0/0/0/0/"
  "/3/0/0/0/0/" [label="[0].number.integer.decimal\n42"]
  "/3/0/0/0/0/" -> "/3/0/0/0/0/0/"
  "/3/0/0/0/0/0/" [label="[0].number.plusMinus\n"]
  "/3/0/0/0/0/" -> "/3/0/0/0/0/1/"
  "/3/0/0/0/0/1/" [label="[1].number.unsigned.integer.decimal\n42"]
  "/" -> "/4/"
  "/4/" [label="[4].Fern.LabeledItem\n[]\\n  "]
  "/4/" -> "/4/0/"
  "/4/0/" [label="[0].Fern\n[]\\n  "]
  "/" -> "/5/"
  "/5/" [label="[5].Fern.LabeledItem\n[\\n    ... \\n  ]\\n"]
  "/5/" -> "/5/0/"
  "/5/0/" [label="[0].Fern\n[\\n    ... \\n  ]\\n"]
  "/5/0/" -> "/5/0/0/"
  "/5/0/0/" [label="[0].Fern.LabeledItem\n1\\n    "]
  "/5/0/0/" -> "/5/0/0/0/"
  "/5/0/0/0/" [label="[0].Fern.value\n1"]
  "/5/0/0/0/" -> "/5/0/0/0/0/"
  "/5/0/0/0/0/" [label="[0].number\n1"]
  "/5/0/0/0/0/" -> "/5/0/0/0/0/0/"
  "/5/0/0/0/0/0/" [label="[0].number.integer\n1"]
  "/5/0/0/0/0/0/" -> "/5/0/0/0/0/0/0/"
  "/5/0/0/0/0/0/0/" [label="[0].number.integer.decimal\n1"]
  "/5/0/0/0/0/0/0/" -> "/5/0/0/0/0/0/0/0/"
  "/5/0/0/0/0/0/0/0/" [label="[0].number.plusMinus\n"]
  "/5/0/0/0/0/0/0/" -> "/5/0/0/0/0/0/0/1/"
  "/5/0/0/0/0/0/0/1/" [label="[1].number.unsigned.integer.decimal\n1"]
  "/5/0/" -> "/5/0/1/"
  "/5/0/1/" [label="[1].Fern.LabeledItem\n'two' : 2\\n    "]
  "/5/0/1/" -> "/5/0/1/0/"
  "/5/0/1/0/" [label="[0].Fern.label\n'two'"]
  "/5/0/1/0/" -> "/5/0/1/0/0/"
  "/5/0/1/0/0/" [label="[0].string\n'two'"]
  "/5/0/1/" -> "/5/0/1/1/"
  "/5/0/1/1/" [label="[1].Fern.value\n2"]
  "/5/0/1/1/" -> "/5/0/1/1/0/"
  "/5/0/1/1/0/" [label="[0].number\n2"]
  "/5/0/1/1/0/" -> "/5/0/1/1/0/0/"
  "/5/0/1/1/0/0/" [label="[0].number.integer\n2"]
  "/5/0/1/1/0/0/" -> "/5/0/1/1/0/0/0/"
  "/5/0/1/1/0/0/0/" [label="[0].number.integer.decimal\n2"]
  "/5/0/1/1/0/0/0/" -> "/5/0/1/1/0/0/0/0/"
  "/5/0/1/1/0/0/0/0/" [label="[0].number.plusMinus\n"]
  "/5/0/1/1/0/0/0/" -> "/5/0/1/1/0/0/0/1/"
  "/5/0/1/1/0/0/0/1/" [label="[1].number.unsigned.integer.decimal\n2"]
  "/5/0/" -> "/5/0/2/"
  "/5/0/2/" [label="[2].Fern.LabeledItem\n3\\n  "]
  "/5/0/2/" -> "/5/0/2/0/"
  "/5/0/2/0/" [label="[0].Fern.value\n3"]
  "/5/0/2/0/" -> "/5/0/2/0/0/"
  "/5/0/2/0/0/" [label="[0].number\n3"]
  "/5/0/2/0/0/" -> "/5/0/2/0/0/0/"
  "/5/0/2/0/0/0/" [label="[0].number.integer\n3"]
  "/5/0/2/0/0/0/" -> "/5/0/2/0/0/0/0/"
  "/5/0/2/0/0/0/0/" [label="[0].number.integer.decimal\n3"]
  "/5/0/2/0/0/0/0/" -> "/5/0/2/0/0/0/0/0/"
  "/5/0/2/0/0/0/0/0/" [label="[0].number.plusMinus\n"]
  "/5/0/2/0/0/0/0/" -> "/5/0/2/0/0/0/0/1/"
  "/5/0/2/0/0/0/0/1/" [label="[1].number.unsigned.integer.decimal\n3"]
})";

    const string_t result_graphviz = SILVA_REQUIRE(pt->span().to_graphviz());
    CHECK(result_graphviz == expected_parse_tree_str_graphviz.substr(1));
  }
}
