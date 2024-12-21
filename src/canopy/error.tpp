#include "error.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;
using enum error_level_t;

TEST_CASE("error", "[error_t]")
{
  error_context_t error_context;

  silva::error_t final_error;
  {
    auto a_1 = make_error(MINOR, "scope a 1"_sov);
    auto a_2 = make_error(MINOR, "scope a 2"_sov);

    silva::error_t a_3;
    {
      auto b_1 = make_error(MINOR, "scope b 1"_sov);
      auto b_2 = make_error(MINOR, "scope b 2"_sov);
      a_3      = make_error(MAJOR, "combined"_sov, std::move(b_1), std::move(b_2));
    }

    auto a_4 = make_error(MINOR, "scope a 4"_sov);

    vector_t<silva::error_t> errors;
    errors.push_back(std::move(a_1));
    errors.push_back(std::move(a_2));
    errors.push_back(std::move(a_3));
    errors.push_back(std::move(a_4));
    final_error = make_error(MINOR, "scope final"_sov, span_t{errors});
  }

  CHECK(error_context.tree.nodes.size() == 7);
  const string_view_t expected = R"(
scope final
  scope a 4
  combined
    scope b 2
    scope b 1
  scope a 2
  scope a 1
)";
  CHECK(final_error.to_string() == expected.substr(1));
}
