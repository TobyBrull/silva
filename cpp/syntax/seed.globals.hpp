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

number_special = [ '-inf' '+inf' 'inf' 'nan' ]
number_plain = [ '+' '-' ] ? DIGIT [ DIGIT ID_LOWER ID_UPPER '.' '\'' '+' '-' ] *

#number_int = [ '+' '-' ] ? DIGIT +
#number_float = [ '+' '-' ] ? DIGIT * '.' [ DIGIT ID_LOWER ID_UPPER '.' '\'' '+' '-' ] *

number = [ number_special number_plain ]


# For example: 2026-03-07
date = DIGIT{4} '-' DIGIT{2} '-' DIGIT{2}                             not DIGIT

# For example: 23:56:00/123.456
time_of_day = ( DIGIT{2} ':' DIGIT{2} ⇒ ':' DIGIT{2}
              ⇒ '/' DIGIT{3} ⇒ '.' DIGIT{3} ⇒ '.' DIGIT{3} )          not DIGIT

# For example: 23:56:00.123456
time_of_day_rfc = ( DIGIT{2} ':' DIGIT{2} ⇒ ':' DIGIT{2}
                  ⇒ '.' DIGIT{3} ⇒ DIGIT{3} ⇒ DIGIT{3} )              not DIGIT

time_of_day_any = ( DIGIT{2} ':' DIGIT{2} ⇒ ':' DIGIT{2}
                  ⇒ ( '.' | '/' ) DIGIT{3} ⇒ DIGIT{3} ⇒ DIGIT{3} )    not DIGIT

timezone = ( 'Z' | ( '+' | '-' ) DIGIT{2} ':' DIGIT{2} not DIGIT )

timepoint_local     = date '/' time_of_day
timepoint_local_rfc = date ( 'T' | ' ' ) time_of_day_rfc
timepoint_local_any = timepoint_local | timepoint_local_rfc

# 2026-03-07/23:56/Z
timepoint     = timepoint_local     '/' timezone
timepoint_rfc = timepoint_local_rfc     timezone
timepoint_any = timepoint | timepoint_rfc


skip_free_form = [ SPACE LINEFEED COMMENT WHITESPACE INDENT DEDENT NEWLINE ] *
skip_off_side  = [ SPACE LINEFEED COMMENT WHITESPACE ] *

None = ε
)'";
}
