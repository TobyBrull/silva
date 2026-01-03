// clang-format off
#include "canopy/unicode.hpp"

namespace silva {
  enum class fragment_category_t {
    Forbidden = 0,
    Newline = 1,
    Space = 2,
    Operator = 3,
    ParenthesisLeft = 4,
    ParenthesisRight = 5,
    XID_Continue = 6,
    XID_Start = 7,
  };

  extern unicode::table_t<fragment_category_t> fragment_table;
}
