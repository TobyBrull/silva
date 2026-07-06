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
  unsigned:
    integer:
      grouping = no_node '\''
      binary = '0b' ( DIGIT | grouping ) *
      octal = '0o' ( DIGIT | grouping ) *
      hexadecimal = '0x' ( DIGIT | ID_LOWER | ID_UPPER | grouping ) *
      decimal = DIGIT ( DIGIT | grouping ) *

    float:
      special = [ 'inf' 'nan' ]
      ⊙ = special | integerPart ( exponent | fraction exponent ? )
      integerPart = integer.decimal
      fraction = '.' DIGIT +
      exponent = 'e' plusMinus DIGIT +

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
