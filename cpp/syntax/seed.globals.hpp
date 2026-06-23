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

identifier = ID_START ID_CONTINUE *
identifier_with_dash = ID_START ( ID_CONTINUE | '-' ) *
identifier_kebab_case = ID_LOWER + ( '-' ID_LOWER + ) *       not ID_CONTINUE
identifier_snake_case = ID_LOWER + ( '_' ID_LOWER + ) *       not ID_CONTINUE
identifier_camel_case = ID_LOWER + ( ID_UPPER ID_LOWER + ) *  not ID_CONTINUE
identifier_pascal_case = ( ID_UPPER ID_LOWER + ) +            not ID_CONTINUE
identifier_macro_case = ID_UPPER + ( '_' ID_UPPER + ) *       not ID_CONTINUE

none = 'none'

boolean = [ 'true' 'false' ]

plus_minus = [ '-' '+' ] ?
number_decimal = plus_minus

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

number_special = [ '-inf' '+inf' 'inf' 'nan' ]
number_plain = [ '+' '-' ] ? DIGIT [ DIGIT ID_LOWER ID_UPPER '.' '\'' '+' '-' ] *

number = [ number_special number_plain ]

# date:             2026-03-07
# time_of_day:      23:56:04/123.456
# timezone:         America/New_York
# timepoint_local:  2026-03-07/23:56:04/123.456
# timepoint:        2026-03-07/23:56:04/123.456/America/New_York
date            = DIGIT{4} '-' DIGIT{2} '-' DIGIT{2} not DIGIT
time_of_day     = ( DIGIT{2} ':' DIGIT{2} ⇒ ':' DIGIT{2}
                    ⇒ '/' DIGIT{3} ⇒ '.' DIGIT{3} ⇒ '.' DIGIT{3} ) not DIGIT
timezone        = ( 'Z' | 'UTC' | ( '+' | '-' ) DIGIT{2} ':' DIGIT{2} not DIGIT )
timepoint_local = date '/' time_of_day
timepoint       = timepoint_local '/' timezone

# time_of_day_rfc:  23:56:00.123456
time_of_day_rfc     = ( DIGIT{2} ':' DIGIT{2} ⇒ ':' DIGIT{2}
                        ⇒ '.' DIGIT{3} ⇒ DIGIT{3} ⇒ DIGIT{3} ) not DIGIT
timepoint_local_rfc = date ( 'T' | ' ' ) time_of_day_rfc
timepoint_rfc       = timepoint_local_rfc timezone

time_of_day_any     = ( DIGIT{2} ':' DIGIT{2} ⇒ ':' DIGIT{2}
                        ⇒ ( '.' | '/' ) DIGIT{3} ⇒ DIGIT{3} ⇒ DIGIT{3} ) not DIGIT
timepoint_local_any = timepoint_local | timepoint_local_rfc
timepoint_any       = timepoint | timepoint_rfc

# 

skip_free_form = [ SPACE LINEFEED COMMENT WHITESPACE INDENT DEDENT NEWLINE ] *
skip_off_side  = [ SPACE LINEFEED COMMENT WHITESPACE ] *

None = ε
)'";
}
