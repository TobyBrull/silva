#pragma once

#include "canopy/string.hpp"

namespace silva::toml {
  const string_view_t seed_str = R"'(
tokenizer Toml:
  include tokenizer FreeForm

language Toml:
  ⊙ = ( Keyval | Table ) *
  Keyval = Key '=' Val
  Key = SimpleKey ( '.' SimpleKey ) *
  SimpleKey = string | identifier
  Val = string | 'true' | 'false' | Array | InlineTable | DateTime | Number
  Array = '[' ( Val ( ε ',' Val ) * ',' ? ) ? ']'
  Table = ArrayTable | StdTable
  StdTable = '[' Key ']'
  ArrayTable = ε '[' '[' Key ']' ']'
  InlineTable = '{' ( Keyval ( ',' Keyval ) * ',' ? ) ? '}'
  Number = ( '-' | '+' ) ? ( number | 'inf' | 'nan' )

  DateTime = date_time | date
  #  ;; Date and Time (as defined in RFC 3339)
  #
  #  date-time      = offset-date-time / local-date-time / local-date / local-time
  #
  #  date-fullyear  = 4DIGIT
  #  date-month     = 2DIGIT  ; 01-12
  #  date-mday      = 2DIGIT  ; 01-28, 01-29, 01-30, 01-31 based on month/year
  #  time-delim     = "T" / %x20 ; T, t, or space
  #  time-hour      = 2DIGIT  ; 00-23
  #  time-minute    = 2DIGIT  ; 00-59
  #  time-second    = 2DIGIT  ; 00-58, 00-59, 00-60 based on leap second rules
  #  time-secfrac   = "." 1*DIGIT
  #  time-numoffset = ( "+" / "-" ) time-hour ":" time-minute
  #  time-offset    = "Z" / time-numoffset
  #
  #  partial-time   = time-hour ":" time-minute [ ":" time-second [ time-secfrac ] ]
  #  full-date      = date-fullyear "-" date-month "-" date-mday
  #  full-time      = partial-time time-offset
  #
  #  ;; Offset Date-Time
  #
  #  offset-date-time = full-date time-delim full-time
  #
  #  ;; Local Date-Time
  #
  #  local-date-time = full-date time-delim partial-time
  #
  #  ;; Local Date
  #
  #  local-date = full-date
  #
  #  ;; Local Time
  #
  #  local-time = partial-time
)'";
}
