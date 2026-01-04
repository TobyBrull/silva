// GENERATED: DO NOT EDIT!
//
// clang-format off
#include "canopy/hash.hpp"
#include "canopy/unicode.hpp"

namespace silva {
  enum class codepoint_category_t {
    Forbidden = 0,
    Newline = 1,
    Space = 2,
    Operator = 3,
    ParenthesisLeft = 4,
    ParenthesisRight = 5,
    XID_Continue = 6,
    XID_Start = 7,
  };

  extern unicode::table_t<codepoint_category_t> codepoint_category_table;

  extern hash_map_t<unicode::codepoint_t, unicode::codepoint_t> opposite_parenthesis;

}
