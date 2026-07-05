#pragma once

#include "canopy/string.hpp"

namespace silva::seed {

  const string_view_t globals_str = R"'(
string = STRING
indent = INDENT
dedent = DEDENT
newline = NEWLINE

parenthesis = PARENTHESIS
operator:
  single = OPERATOR
  greedy = OPERATOR +

identifier:
  ⊙ = ID_START ID_CONTINUE *
  with_dashes = ID_START ( ID_CONTINUE | '-' ) *
  kebab_case = ID_LOWER + ( '-' [ ID_LOWER DIGIT ] + ) *      not ID_CONTINUE
  snake_case = [ '_' ID_LOWER DIGIT ] +                       not ID_CONTINUE
  camel_case = ID_LOWER + ( ID_UPPER ID_LOWER + ) *           not ID_CONTINUE
  pascal_case = ( ID_UPPER ID_LOWER + ) +                     not ID_CONTINUE
  macro_case = ID_UPPER + ( '_' ID_UPPER + ) *                not ID_CONTINUE

None = "none"

Boolean = [ "true" "false" ]

number:
  grouping = [ '\'' '_' ]

  unsigned:
    integer:
      binary:
        digit = [ '0' '1' ]
        ⊙ = '0b' digit ( digit | grouping digit ) *
      octal:
        digit = [ '0' '1' '2' '3' '4' '5' '6' '7' ]
        ⊙ = '0o' digit ( digit | grouping digit ) *
      hexadecimal:
        digit = decimal.digit | [ 'a' 'b' 'c' 'd' 'e' 'f' 'A' 'B' 'C' 'D' 'E' 'F' ]
        ⊙ = '0x' digit ( digit | grouping digit ) *
      decimal:
        digit = octal.digit | [ '8' '9' ]
        ⊙ = digit ( digit | grouping digit ) *

    float:
      special = [ 'inf' 'nan' ]

      ⊙ = special | integer_part ( exponent | fraction exponent ? )

      integer_part = integer.decimal
      fraction = '.' integer.decimal.digit +
      exponent = 'e' plus_minus integer.decimal.digit +

  plus_minus = [ '-' '+' ] ?

  integer:
    binary = plus_minus unsigned.integer.binary
    octal = plus_minus unsigned.integer.octal
    hexadecimal = plus_minus unsigned.integer.hexadecimal
    decimal = plus_minus unsigned.integer.decimal
    ⊙ = [ binary octal hexadecimal decimal ]

  float:
    special = plus_minus unsigned.float.special
    ⊙ = plus_minus unsigned.float

  ⊙ = [ float float.special integer ]

# date:               2026-03-07
date = DIGIT{4} '-' DIGIT{2} '-' DIGIT{2} not DIGIT

# time.of_day:        23:56:04/123.456
# time.zone:          America/New_York
# time.point.local:   2026-03-07/23:56:04/123.456
# time.point:         2026-03-07/23:56:04/123.456/America/New_York
time:
  of_day:
    ⊙ = ( DIGIT{2} ':' DIGIT{2} ⇒ ':' DIGIT{2} ⇒ '/' DIGIT{3} ⇒ '.' DIGIT{3} ⇒ '.' DIGIT{3} ) not DIGIT
    # time_of_day_rfc:  23:56:00.123456
    rfc     = ( DIGIT{2} ':' DIGIT{2} ⇒ ':' DIGIT{2} ⇒ '.' DIGIT{3} ⇒ DIGIT{3} ⇒ DIGIT{3} ) not DIGIT
    any     = ( DIGIT{2} ':' DIGIT{2} ⇒ ':' DIGIT{2} ⇒ ( '.' | '/' ) DIGIT{3} ⇒ DIGIT{3} ⇒ DIGIT{3} ) not DIGIT

  zone = ( 'Z' | 'UTC' | ( '+' | '-' ) DIGIT{2} ':' DIGIT{2} not DIGIT )

  point:
    ⊙ = local '/' time.zone
    rfc = local.rfc time.zone
    any = time.point | time.point.rfc

    local:
      ⊙ = date '/' time.of_day
      rfc = date ( 'T' | ' ' ) time.of_day.rfc
      any = time.point.local | time.point.local.rfc

skip:
  free_form = [ SPACE LINEFEED COMMENT WHITESPACE INDENT DEDENT NEWLINE ] *
  off_side  = [ SPACE LINEFEED COMMENT WHITESPACE ] *

Epsilon = ε
)'";
}
