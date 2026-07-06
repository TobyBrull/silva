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
  withDashes = ID_START ( ID_CONTINUE | '-' ) *
  kebabCase = ID_LOWER + ( '-' [ ID_LOWER DIGIT ] + ) *       not ID_CONTINUE
  snakeCase = [ '_' ID_LOWER DIGIT ] +                        not ID_CONTINUE
  camelCase = ID_LOWER + ( ID_UPPER ID_LOWER + ) *            not ID_CONTINUE
  pascalCase = ( ID_UPPER ID_LOWER + ) +                      not ID_CONTINUE
  macroCase = ID_UPPER + ( '_' ID_UPPER + ) *                 not ID_CONTINUE

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

      ⊙ = special | integerPart ( exponent | fraction exponent ? )

      integerPart = integer.decimal
      fraction = '.' integer.decimal.digit +
      exponent = 'e' plusMinus integer.decimal.digit +

  plusMinus = [ '-' '+' ] ?

  integer:
    binary = plusMinus unsigned.integer.binary
    octal = plusMinus unsigned.integer.octal
    hexadecimal = plusMinus unsigned.integer.hexadecimal
    decimal = plusMinus unsigned.integer.decimal
    ⊙ = [ binary octal hexadecimal decimal ]

  float:
    special = plusMinus unsigned.float.special
    ⊙ = plusMinus unsigned.float

  ⊙ = [ float float.special integer ]

# date:               2026-03-07
date = DIGIT{4} '-' DIGIT{2} '-' DIGIT{2} not DIGIT

# time.ofDay:         23:56:04/123.456
# time.zone:          America/New_York
# time.point.local:   2026-03-07/23:56:04/123.456
# time.point:         2026-03-07/23:56:04/123.456/America/New_York
time:
  ofDay:
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
      ⊙ = date '/' time.ofDay
      rfc = date ( 'T' | ' ' ) time.ofDay.rfc
      any = time.point.local | time.point.local.rfc

skip:
  freeForm = [ SPACE LINEFEED COMMENT WHITESPACE INDENT DEDENT NEWLINE ] *
  offSide  = [ SPACE LINEFEED COMMENT WHITESPACE ] *

Epsilon = ε
)'";
}
