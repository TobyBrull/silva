#pragma once

#include "canopy/string.hpp"

namespace silva::seed {

  const string_view_t globals_str = R"'(
string:
  ⊙ = MULTILINE_STRING | single | double
  single = no_node '\'' ~ ( '\\' ANY | not '\'' ANY ) * '\''
  double = no_node '"' ~ ( '\\' ANY | not '"' ANY ) * '"'

parenthesis = PARENTHESIS
operator:
  single = no_node OPERATOR
  greedy = no_node OPERATOR +

identifier:
  ⊙ = ID_START ID_CONTINUE *
  withDashes  = no_node ID_START ( ID_CONTINUE | '-' ) *
  kebabCase   = no_node ID_LOWER [ '-' ID_LOWER DIGIT ] *           not ID_CONTINUE
  snakeCase   = no_node [ '_' ID_LOWER DIGIT ] +                    not ID_CONTINUE
  camelCase   = no_node ID_LOWER [ ID_UPPER ID_LOWER DIGIT ] *      not ID_CONTINUE
  pascalCase  = no_node ID_UPPER [ ID_UPPER ID_LOWER DIGIT ] *      not ID_CONTINUE
  macroCase   = no_node ID_UPPER [ '_' ID_UPPER DIGIT ] +           not ID_CONTINUE

none = "none"

boolean = [ "true" "false" ]

number:
  unsigned:
    integer:
      grouping = no_node '_'
      binary = '0b' ( DIGIT | grouping ) *
      octal = '0o' ( DIGIT | grouping ) *
      hexadecimal = '0x' ( DIGIT | ID_LOWER | ID_UPPER | grouping ) *
      decimal = DIGIT ( DIGIT | grouping ) *

    float:
      special = [ 'inf' 'nan' ]
      ⊙ = special | integerPart ( exponent | fraction exponent ? )
      integerPart = integer.decimal
      fraction = '.' integer.decimal
      exponent = 'e' plusMinus ? integer.decimal

  plusMinus = [ '+' '-' ]

  integer:
    binary = plusMinus ? unsigned.integer.binary
    octal = plusMinus ? unsigned.integer.octal
    hexadecimal = plusMinus ? unsigned.integer.hexadecimal
    decimal = plusMinus ? unsigned.integer.decimal
    ⊙ = [ binary octal hexadecimal decimal ]

  float:
    special = plusMinus ? unsigned.float.special
    ⊙ = plusMinus ? unsigned.float

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

comment = no_node '#' ( LANGUAGE | ANY ) *

indent  = no_node INDENT .offSide.blankLines
dedent  = no_node DEDENT .offSide.blankLines
newline = no_node NEWLINE .offSide.blankLines

offSide:
  ⊙ = horizontal
  horizontal = no_node [ SPACE LINE_CONTINUATION comment ] *
  blankLines = no_node ( horizontal NEWLINE ) *

freeForm = no_node [ comment \
                     NEWLINE SPACE LINE_CONTINUATION \
                     INDENT DEDENT INDENTATION_BROKEN ] *

Epsilon = ε
)'";
}
