* make tests exhaustive
* support:
    * "a < b < c"
    * parentheses can be:
        * Primary:              "a + ( b + c )"     -> "{ + a '(b+c)' }"
                                "a ( b + c )"       -> "{ Concat a '(b+c)' }"
        * transparent parens:   "a + ( b + c )"     -> "{ + a { + b c } }"
        * Postfix:              "a ( b, c )"        -> "{ FuncCall a b c}"
