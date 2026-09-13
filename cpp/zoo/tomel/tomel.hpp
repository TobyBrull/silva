#pragma once

#include "canopy/string.hpp"

namespace silva::tomel {

  // Adoption of
  // https://github.com/toml-lang/toml/blob/main/toml.abnf
  // with some differences with respect to how literals are handled
  //
  const string_view_t seed_str = R"'(
language Tomel:
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
