#pragma once

#include "canopy/string.hpp"

namespace silva::seed {

  const string_view_t globals_str = R"'(
string = STRING
indent = INDENT
dedent = DEDENT
newline = NEWLINE

parenthesis = PARENTHESIS
operator_single = OPERATOR
operator_greedy = OPERATOR +

identifier:
  ⊙ = ID_START ID_CONTINUE *
  with_dashes = ID_START ( ID_CONTINUE | '-' ) *
  kebab_case = ID_LOWER + ( '-' ID_LOWER + ) *       not ID_CONTINUE
  snake_case = ID_LOWER + ( '_' ID_LOWER + ) *       not ID_CONTINUE
  camel_case = ID_LOWER + ( ID_UPPER ID_LOWER + ) *  not ID_CONTINUE
  pascal_case = ( ID_UPPER ID_LOWER + ) +            not ID_CONTINUE
  macro_case = ID_UPPER + ( '_' ID_UPPER + ) *       not ID_CONTINUE

none = 'none'

boolean = [ 'true' 'false' ]

number: 
  plus_minus = [ '-' '+' ] ?
  ⊙ = [ number.special number.plain ]

  special = [ '-inf' '+inf' 'inf' 'nan' ]
  plain = [ '+' '-' ] ? DIGIT [ DIGIT ID_LOWER ID_UPPER '.' '\'' '+' '-' ] *

  #  integer = dec-int / hex-int / oct-int / bin-int
  #  
  #  minus = %x2D                       ; -
  #  plus = %x2B                        ; +
  #  underscore = %x5F                  ; _
  #  digit1-9 = %x31-39                 ; 1-9
  #  digit0-7 = %x30-37                 ; 0-7
  #  digit0-1 = %x30-31                 ; 0-1
  #  
  #  hex-prefix = %x30.78               ; 0x
  #  oct-prefix = %x30.6F               ; 0o
  #  bin-prefix = %x30.62               ; 0b
  #  
  #  dec-int = [ minus / plus ] unsigned-dec-int
  #  unsigned-dec-int = DIGIT / digit1-9 1*( DIGIT / underscore DIGIT )
  #  
  #  hex-int = hex-prefix HEXDIG *( HEXDIG / underscore HEXDIG )
  #  oct-int = oct-prefix digit0-7 *( digit0-7 / underscore digit0-7 )
  #  bin-int = bin-prefix digit0-1 *( digit0-1 / underscore digit0-1 )
  #  
  #  ;; Float
  #  
  #  float = float-int-part ( exp / frac [ exp ] )
  #  float =/ special-float
  #  
  #  float-int-part = dec-int
  #  frac = decimal-point zero-prefixable-int
  #  decimal-point = %x2E               ; .
  #  zero-prefixable-int = DIGIT *( DIGIT / underscore DIGIT )
  #  
  #  exp = "e" float-exp-part
  #  float-exp-part = [ minus / plus ] zero-prefixable-int
  #  
  #  #number_int = [ '+' '-' ] ? DIGIT +
  #  #number_float = [ '+' '-' ] ? DIGIT * '.' [ DIGIT ID_LOWER ID_UPPER '.' '\'' '+' '-' ] *

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

None = ε
)'";
}
