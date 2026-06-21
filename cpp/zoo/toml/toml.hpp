#pragma once

#include "canopy/string.hpp"

namespace silva::toml {
  const string_view_t seed_str = R"'(
language Toml:
  skip = skip_free_form

  ⊙ = ( Keyval | Table ) *
  Keyval = Key '=' Val
  Key = SimpleKey ( '.' SimpleKey ) *
  SimpleKey = string | identifier_with_dash
  Val = string | 'true' | 'false' | Array | InlineTable \
      | timepoint_any | timepoint_local_any | date | time_of_day_any \
      | number
  Array = '[' ( Val ( ε ',' Val ) * ',' ? ) ? ']'
  Table = ArrayTable | StdTable
  StdTable = '[' Key ']'
  ArrayTable = '[[' Key ']]'
  InlineTable = '{' ( Keyval ( ',' Keyval ) * ',' ? ) ? '}'
)'";
}
