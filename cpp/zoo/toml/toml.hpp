#pragma once

#include "canopy/string.hpp"

namespace silva::toml {
  const string_view_t seed_str = R"'(
language Toml:
  skip = skip_free_form

  identifier = ID_START ( ID_CONTINUE | '-' | '_' ) *
  date = DIGIT{4} '-' DIGIT{2} '-' DIGIT{2}
  time = DIGIT{2} ':' DIGIT{2} ( ':' DIGIT{2} ( '.' DIGIT{3} ( DIGIT{3} ( DIGIT{3} ) ? ) ? ) ? ) ?
  timezone = ( 'Z' | ( '+' | '-' ) DIGIT{2} ':' DIGIT{2} )
  date_time = date ( ' ' | 'T' ) time timezone ?

  ⊙ = ( Keyval | Table ) *
  Keyval = Key '=' Val
  Key = SimpleKey ( '.' SimpleKey ) *
  SimpleKey = string | identifier
  Val = string | 'true' | 'false' | Array | InlineTable | date_time | date | time | Number
  Array = '[' ( Val ( ε ',' Val ) * ',' ? ) ? ']'
  Table = ArrayTable | StdTable
  StdTable = '[' Key ']'
  ArrayTable = '[[' Key ']]'
  InlineTable = '{' ( Keyval ( ',' Keyval ) * ',' ? ) ? '}'
  Number = ( '-' | '+' ) ? ( number | 'inf' | 'nan' )
)'";
}
