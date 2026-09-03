#pragma once

#include "canopy/types.hpp"

#include "syntax/seed_interpreter.hpp"

namespace silva::pine {

  // Adoption of the full grammar of Python, see
  // https://docs.python.org/3/reference/grammar.html
  //
  // Deviations from the reference grammar:
  //  * Python's exponentiation operator '**' is asymmetric in terms of its precedence relative to
  //    the unary '-' in the sense that the expression « -2**2 » is parsed as « -(2**2) » whereas «
  //    2**-2 » is parsed as « 2**(-2) ». This parser here rejects the second form as unary '-' has
  //    lower precedence than '**'.
  //  * The two-token operators "not in" and "is not" are single literals here, so their two words
  //    have to be separated by exactly one space; « a not  in b » is not accepted.
  //  * The many rules spelling out where '/', '*' or '**' may appear in a parameter-list
  //    ("slash_no_default", "star_etc", "kwds", their "lambda_"-variants, ...) are collapsed onto
  //    "Params"/"LambdaParams", which leaves the ordering of the parameter kinds unchecked.
  //  * The rules dealing with assignment-targets ("t_primary", "star_atom", "del_t_atom", ...) are
  //    collapsed onto "Expr.Primary". This accepts a few targets that Python rejects (e.g.,
  //    « f(x) = 1 ») but parses the same language otherwise.
  //
  const string_view_t seed_str = R"'(
language Pine:
  skip = skip.offSide

  ⊙ = Stmt *

  Stmt:
    ⊙ = Compound | Simples

    Simples = Simple ( ε ';' Simple ) * ';' ? newline
    Simple = [ Return Import Raise Pass Del Yield Assert Break Continue Global Nonlocal
               Assignment TypeAlias StarExprs ]

    Compound = [ Function If Class With For Try While Match ]

    Assignment:
      ⊙ = Annotated | Plain | Augmented
      Annotated = Expr.Primary ':' Expr ( '=' Rhs ) ?
      Plain = ( Target.Stars '=' not '=' ) + Rhs
      Augmented = Expr.Primary augassign Rhs
      augassign = [ '+=' '-=' '*=' '@=' '/=' '%=' '&=' '|=' '^=' '<<=' '>>=' '**=' '//=' ]
      Rhs = Expr.Yield | StarExprs

    Return = "return" StarExprs ?
    Raise = "raise" ⇒ Expr ⇒ "from" Expr
    Pass = "pass"
    Break = "break"
    Continue = "continue"
    Global = "global" identifier ( ε ',' identifier ) *
    Nonlocal = "nonlocal" identifier ( ε ',' identifier ) *
    Del = "del" Expr.Primary ( ε ',' Expr.Primary ) * ',' ?
    Yield = Expr.Yield
    Assert = "assert" Expr ( ',' Expr ) ?

    Import:
      ⊙ = Name | From
      Name = "import" DottedAsName ( ε ',' DottedAsName ) *
      From = "from" ( dots ? DottedName "import" | dots "import" ) ~ Targets
      dots = '.' +
      Targets = '(' AsName ( ε ',' AsName ) * ',' ? ')' \
              | AsName ( ε ',' AsName ) * \
              | '*'
      AsName = identifier ( "as" identifier ) ?
      DottedAsName = DottedName ( "as" identifier ) ?
      DottedName = identifier ( ε '.' identifier ) *

    Block = newline indent Stmt + dedent | Simples
    Decorators = ( '@' Expr.Named newline ) +
    async = "async"

    Class = Decorators ? "class" ~ identifier TypeParams ( '(' Arguments ')' ) ? ':' Block

    Function:
      ⊙ = Decorators ? async ? "def" ~ identifier TypeParams '(' Params ')' ( '->' Expr ) ? ':' Block
      Params = Param ( ε ',' Param ) * ',' ? | ε
      Param = '/' | '**' ParamDef | '*' ParamDefStar ? | ParamDef
      ParamDef = identifier ( ':' Expr ) ? ( '=' Expr ) ?
      ParamDefStar = identifier ( ':' StarExpr ) ? ( '=' Expr ) ?

    TypeAlias = "type" identifier TypeParams '=' Expr

    If = "if" Expr.Named ':' Block Elif * Else ?
    Elif = "elif" Expr.Named ':' Block
    Else = "else" ':' Block

    While = "while" Expr.Named ':' Block Else ?
    For = async ? "for" ~ Target.Stars "in" StarExprs ':' Block Else ?

    With:
      ⊙ = async ? "with" ~ Items ':' Block
      Items = Item ( ε ',' Item ) * | ε '(' Item ( ε ',' Item ) * ',' ? ')'
      Item = Expr ( "as" Target.Star ) ?

    Try:
      ⊙ = "try" ':' Block ( ExceptStar + | Except + ) ? Else ? Finally ?
      Except = ε "except" ( Expr ( "as" identifier ) ? ) ? ':' Block
      ExceptStar = ε "except" '*' Expr ( "as" identifier ) ? ':' Block
      Finally = "finally" ':' Block

    Match = ε "match" SubjectExpr ':' newline indent Case + dedent
    Case = ε "case" Pattern.Patterns Guard ? ':' Block
    Guard = "if" Expr.Named
    SubjectExpr = StarNamedExpr ',' StarNamedExprs ? | Expr.Named

  TypeParams:
    ⊙ = '[' Singular ( ε ',' Singular ) * ',' ? ']' | ε
    Singular = no_node Normal | Star2 | Star1
    Normal = identifier ( ':' Expr ) ? Default ?
    Star1 = '*' identifier ( '=' StarExpr ) ?
    Star2 = '**' identifier Default ?
    Default = '=' Expr

  LambdaParams = ( LambdaParam ( ε ',' LambdaParam ) * ',' ? ) ?
  LambdaParam = '/' | '**' LambdaParamDef | '*' LambdaParamDef ? | LambdaParamDef
  LambdaParamDef = identifier ( '=' not '=' Expr ) ?

  Arguments:
    ⊙ = Singular ( ε ',' Singular ) * ',' ? | ε
    Singular = '**' Expr | '*' Expr | identifier '=' not '=' Expr | Expr.Named ForIfClauses ?

  Slices = Slice ( ε ',' Slice ) * ',' ?
  Slice = Expr ? ':' not '=' Expr ? ( ':' not '=' Expr ? ) ? | '*' Expr | Expr.Named

  StarExprs = StarExpr ( ε ',' StarExpr ) * ',' ?
  StarExpr = no_node Starred | Expr
  Starred = '*' Expr.BitOr
  StarNamedExprs = StarNamedExpr ( ε ',' StarNamedExpr ) * ',' ?
  StarNamedExpr = no_node Starred | Expr.Named

  ForIfClauses = ForIfClause +
  ForIfClause = "async" ? "for" Target.Stars "in" Expr.Disjunction ( "if" Expr.Disjunction ) *

  Target:
    Stars = Star ( ε ',' Star ) * ',' ?
    Star = no_node Starred | Expr.Primary
    Starred = '*' Star

  Expr:
    ⊙ = no_node axe Atom
      Primary     = ltr  postfix_nest -> Arguments '(' ')' \
                         postfix_nest -> Slices '[' ']' \
                         infix '.'
      Await       = rtl  prefix "await"
      Power       = rtl  infix '**'
      Unary       = rtl  prefix '+' '-' '~'
      Term        = ltr  infix '*' '/' '//' '%' '@'
      Sum         = ltr  infix '+' '-'
      Shift       = ltr  infix '<<' '>>'
      BitAnd      = ltr  infix '&'
      BitXor      = ltr  infix '^'
      BitOr       = ltr  infix '|'
      Comparison  = ltr  infix '==' '!=' '<=' '>=' '<' '>' \
                         "in" "not in" "is" "is not"
      Inversion   = rtl  prefix "not"
      Conjunction = ltr  infix_flat "and"
      Disjunction = ltr  infix_flat "or"
      Conditional = rtl  ternary "if" "else"
      Lambda      = rtl  prefix_nest -> LambdaParams "lambda" ':'

    Atom = ( "True" | "False" | "None" | '...' | Strings | number
           | GenExp | Group | Tuple
           | ListComp | List
           | DictComp | SetComp | Dict | Set
           | identifier )
    GenExp = ε '(' Named ForIfClauses ')'
    Group = ε '(' ( Yield | Named ) ')'
    Tuple = ε '(' ( StarNamedExpr ',' StarNamedExprs ? ) ? ')'
    ListComp = ε '[' Named ForIfClauses ']'
    List = '[' StarNamedExprs ? ']'
    DictComp = ε '{' KvPair ForIfClauses '}'
    SetComp = ε '{' Named ForIfClauses '}'
    Dict = ε '{' ( KvPairs ) ? '}'
    Set = '{' StarNamedExprs '}'
    KvPairs = KvPair ( ε ',' KvPair ) * ',' ?
    KvPair = '**' Expr.BitOr | Expr ':' not '=' Expr

    Named = Assign | Expr
    Assign = identifier ':=' Expr
    Yield = "yield" ( "from" Expr | StarExprs ? )

    Strings = stringLiteral +
    stringLiteral = ( ID_START ID_CONTINUE * ) ? STRING

    number:
      ⊙ = [ imaginary float integer ] not ID_CONTINUE
      digits = no_node DIGIT ( DIGIT | '_' ) *
      exponent = no_node [ 'e' 'E' ] [ '+' '-' ] ? digits
      integer = no_node [ binary octal hexadecimal decimal ]
      binary = no_node ε '0' [ 'b' 'B' ] ( DIGIT | '_' ) +
      octal = no_node ε '0' [ 'o' 'O' ] ( DIGIT | '_' ) +
      hexadecimal = no_node ε '0' [ 'x' 'X' ] ( DIGIT | ID_LOWER | ID_UPPER | '_' ) +
      decimal = no_node digits
      float = no_node ( digits '.' digits ? exponent ? | '.' digits exponent ? | digits exponent )
      imaginary = no_node ( float | digits ) [ 'j' 'J' ]

  Pattern:
    ⊙ = As | Or
    Patterns = OpenSequence | Pattern
    As = Or "as" Capture
    Or = Closed ( ε '|' Closed ) *
    Closed = [ Literal Class Value Group Sequence Mapping Wildcard Capture ]

    Literal = ComplexNumber | SignedNumber | Expr.Strings | "None" | "True" | "False"
    SignedNumber = '-' ? Expr.number
    ComplexNumber = SignedNumber [ '+' '-' ] Expr.number

    Wildcard = "_"
    Capture = not "_" identifier not [ '.' '(' '=' ]
    Value = Attr not [ '.' '(' '=' ]
    Attr = identifier ( ε '.' identifier ) +
    NameOrAttr = identifier ( ε '.' identifier ) *

    Group = ε '(' Pattern ')'
    Sequence = ε '[' MaybeSequence ? ']' | '(' OpenSequence ? ')'
    OpenSequence = MaybeStar ',' MaybeSequence ?
    MaybeSequence = MaybeStar ( ε ',' MaybeStar ) * ',' ?
    MaybeStar = Star | Pattern
    Star = '*' ( Capture | Wildcard )

    Mapping = '{' ( Items ( ε ',' DoubleStar ) ? | DoubleStar ) ? ',' ? '}'
    Items = KeyValue ( ε ',' KeyValue ) *
    KeyValue = ( Literal | Attr ) ':' not '=' Pattern
    DoubleStar = '**' Capture

    Class = NameOrAttr '(' ( Positionals ( ε ',' Keywords ) ? | Keywords ) ? ',' ? ')'
    Positionals = Pattern ( ε ',' Pattern ) *
    Keywords = Keyword ( ε ',' Keyword ) *
    Keyword = identifier '=' not '=' Pattern
)'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t);
}
