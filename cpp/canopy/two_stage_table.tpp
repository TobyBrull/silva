#include "unicode.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("two-stage-table", "[two_stage_table_t]")
  {
    const two_stage_table_t<int8_t, std::string, 2> tst{
        .stage_1 = {8, 4, 8},
        .stage_2 = {0, 1, 2, 1, /**/ 2, 2, 1, 0, /**/ 2, 0, 2, 1},
        .stage_3 = {"aa", "bb", "cc"},
    };
    CHECK(tst.key_size() == 0b1100);
    CHECK(tst[0] == "cc");
    CHECK(tst[1] == "aa");
    CHECK(tst[2] == "cc");
    CHECK(tst[3] == "bb");
    CHECK(tst[4] == "cc");
    CHECK(tst[5] == "cc");
    CHECK(tst[6] == "bb");
    CHECK(tst[7] == "aa");
    CHECK(tst[8] == "cc");
    CHECK(tst[9] == "aa");
    CHECK(tst[10] == "cc");
    CHECK(tst[11] == "bb");
    SILVA_REQUIRE(tst.validate());
  }
}
