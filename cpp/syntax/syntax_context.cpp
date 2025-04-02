#include "syntax_context.hpp"

#include "canopy/convert.hpp"
#include "canopy/enum.hpp"

namespace silva {
  using enum token_category_t;

  string_view_t to_string(const token_category_t x)
  {
    static const auto vals = enum_hashmap_to_string<token_category_t>();
    return string_view_t{vals.at(x)};
  }

  expected_t<string_view_t> token_info_t::string_as_plain_contained() const
  {
    SILVA_EXPECT(category == STRING, MAJOR);
    SILVA_EXPECT(str.size() >= 2, MINOR);
    SILVA_EXPECT(str.front() == '\'' && str.back() == '\'', MINOR);
    for (index_t i = 1; i < str.size() - 1; ++i) {
      SILVA_EXPECT(str[i] != '\\' && str[i] != '\'', MINOR);
    }
    return string_view_t{str}.substr(1, str.size() - 2);
  }

  expected_t<double> token_info_t::number_as_double() const
  {
    SILVA_EXPECT(category == NUMBER, MAJOR);
    return convert_to<double>(str);
  }

  hash_value_t hash_impl(const name_info_t& x)
  {
    return hash(tuple_t<name_id_t, token_id_t>{x.parent_name, x.base_name});
  }
}
