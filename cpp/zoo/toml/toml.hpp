#pragma once

#include "canopy/string.hpp"

namespace silva::toml {
  const string_view_t seed_str = R"'(
language Toml:
  skip = skip.freeForm

  ⊙ = ( Keyval | Table ) *
  Keyval = Key '=' Val
  Key = SimpleKey ( '.' SimpleKey ) *
  SimpleKey = string | identifier.withDashes
  Val = string | "true" | "false" | Array | InlineTable \
      | time.point.any | time.point.local.any | date | time.ofDay.any \
      | number
  Array = '[' ( Val ( ε ',' Val ) * ',' ? ) ? ']'
  Table = ArrayTable | StdTable
  StdTable = '[' Key ']'
  ArrayTable = '[[' Key ']]'
  InlineTable = '{' ( Keyval ( ',' Keyval ) * ',' ? ) ? '}'
)'";
}
