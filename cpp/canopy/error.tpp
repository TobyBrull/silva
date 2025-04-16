#include "error.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  using enum error_level_t;

  TEST_CASE("error", "[error_t]")
  {
    error_context_t error_context;

    silva::error_t final_error;
    {
      auto a_1 = make_error(MINOR, {}, "scope a 1");
      auto a_2 = make_error(MINOR, {}, "scope a 2");

      silva::error_t a_3;
      {
        auto b_1 = make_error(MINOR, {}, "scope b 1");
        auto b_2 = make_error(MINOR, {}, "scope b 2");
        std::array<silva::error_t, 2> arr{std::move(b_1), std::move(b_2)};
        a_3 = make_error(MAJOR, arr, "combined");
      }

      auto a_4 = make_error(MINOR, {}, "scope a 4");

      vector_t<silva::error_t> errors;
      errors.push_back(std::move(a_1));
      errors.push_back(std::move(a_2));
      errors.push_back(std::move(a_3));
      errors.push_back(std::move(a_4));
      final_error = make_error(MINOR, errors, "scope final"_sov);
    }

    CHECK(error_context.tree.nodes.size() == 7);
    const string_view_t expected = R"(
  scope a 1
  scope a 2
    scope b 1
    scope b 2
  combined
  scope a 4
scope final
)";
    const auto result            = to_string(final_error);
    CHECK(result.as_string_view() == expected.substr(1));
  }
}
