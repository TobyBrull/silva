* make tests exhaustive
    * PostfixExpr
    * custom parentheses
    * Concat
* support:
    * Concat
    * "a < b < c"
    * parentheses can be:
        * Primary:              "a + ( b + c )"     -> "{ + a '(b+c)' }"
                                "a ( b + c )"       -> "{ Concat a '(b+c)' }"
        * Ternary:              "x ( A ) y"         -> "{ ( x A y }"
        * transparent parens:   "a + ( b + c )"     -> "{ + a { + b c } }"
        * Postfix:              "a ( b, c )"        -> "{ FuncCall a b c}"
