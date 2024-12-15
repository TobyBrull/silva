#include "tokenization.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("tokenization", "[tokenization_t]")
{
  using td_t = tokenization_t::token_data_t;
  using ld_t = tokenization_t::line_data_t;
  {
    const source_code_t source_code("unit-test", "Hello   123 .<>.");
    const tokenization_t tokenization =
        SILVA_EXPECT_REQUIRE(tokenize(const_ptr_unowned(&source_code)));
    CHECK(tokenization.token_datas ==
          std::vector<td_t>({
              td_t{source_code.text.substr(0, 5), token_category_t::IDENTIFIER},
              td_t{source_code.text.substr(8, 3), token_category_t::NUMBER},
              td_t{source_code.text.substr(12, 4), token_category_t::OPERATOR},
          }));
  }

  {
    const source_code_t source_code("unit-test", R"(Silva "Hel\"lo"  .(). # .().
  1 + 3)");
    const tokenization_t tokenization =
        SILVA_EXPECT_REQUIRE(tokenize(const_ptr_unowned(&source_code)));
    CHECK(tokenization.token_datas ==
          std::vector<td_t>({
              td_t{source_code.text.substr(0, 5), token_category_t::IDENTIFIER},
              td_t{source_code.text.substr(6, 9), token_category_t::STRING},
              td_t{source_code.text.substr(17, 1), token_category_t::OPERATOR},
              td_t{source_code.text.substr(18, 1), token_category_t::OPERATOR},
              td_t{source_code.text.substr(19, 1), token_category_t::OPERATOR},

              td_t{source_code.text.substr(31, 1), token_category_t::NUMBER},
              td_t{source_code.text.substr(33, 1), token_category_t::OPERATOR},
              td_t{source_code.text.substr(35, 1), token_category_t::NUMBER},
          }));
  }
}
