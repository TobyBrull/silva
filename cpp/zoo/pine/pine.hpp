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
  //
  //  * The many rules spelling out where a '/', a '*' or a '**' may appear in a parameter-list
  //    ("slash_no_default", "star_etc", "kwds", their "lambda_"-variants, ...) are collapsed onto
  //    "Params"/"LambdaParams", which leaves the ordering of the parameter kinds unchecked.
  //  * The rules dealing with assignment-targets ("t_primary", "star_atom", "del_t_atom", ...) are
  //    collapsed onto "Expr.Primary". This accepts a few targets that Python rejects (e.g.,
  //    « f(x) = 1 ») but parses the same language otherwise.
  //  * Support the two-token operators "not in" and "is not".
  //
  const string_view_t seed_str = R"'(
language Pine:
  skip = skip.offSide

  ⊙ = Stmt *

  Stmt:
    ⊙ = Compound | Simples
    Block = newline indent Stmt + dedent | Simples

    Simples = Simple ( ε ';' Simple ) * ';' ? newline
    Simple = [ Return Import Raise Pass Del Yield Assert Break Continue Global Nonlocal
               TypeAlias Assignment StarExprs ]

    Pass = "pass"
    Break = "break"
    Continue = "continue"
    Return = "return" StarExprs ?
    Raise = "raise" ( Expr ( "from" Expr ) ? ) ?
    Del = "del" Target.Dels
    Yield = Expr.Yield
    Assert = "assert" Expr ( ',' Expr ) ?
    Global = "global" identifier ( ε ',' identifier ) *
    Nonlocal = "nonlocal" identifier ( ε ',' identifier ) *
    TypeAlias = ε "type" identifier TypeParams ? '=' not '=' Expr

    Assignment:
      ⊙ = Annotated | Augmented | Plain
      Annotated = Target.Single ':' not '=' Expr ( '=' not '=' Rhs ) ?
      Augmented = Target.Single augassign Rhs
      Plain = ( Target.Stars '=' not '=' ) + Rhs
      augassign = [ '+=' '-=' '**=' '*=' '@=' '//=' '/=' '%=' '&=' '|=' '^=' '<<=' '>>=' ]
    Rhs = Expr.Yield | StarExprs

    Compound = [ Decorated Async Function Class If While For Try With Match ]
    Decorated = ( '@' Expr.Named newline ) + ( Async | Function | Class )
    Async = "async" [ Function For With ]

    Function = "def" identifier TypeParams ? '(' Params ? ')' ( '->' Expr ) ? ':' Block
    Class = "class" identifier TypeParams ? ( '(' Arguments ')' ) ? ':' Block

    If = "if" Expr.Named ':' Block Elif * Else ?
    Elif = "elif" Expr.Named ':' Block
    Else = "else" ':' Block
    While = "while" Expr.Named ':' Block Else ?
    For = "for" Target.Stars "in" StarExprs ':' Block Else ?

    With = "with" WithItems ':' Block
    WithItems = ε '(' WithItem ( ε ',' WithItem ) * ',' ? ')' | WithItem ( ε ',' WithItem ) *
    WithItem = Expr ( "as" Target.Star ) ?

    Try = "try" ':' Block ( ExceptStar + | Except + ) ? Else ? Finally ?
    Except = ε "except" ( Expr ( "as" identifier ) ? ) ? ':' Block
    ExceptStar = ε "except" '*' Expr ( "as" identifier ) ? ':' Block
    Finally = "finally" ':' Block

    Match = ε "match" SubjectExpr ':' newline indent Case + dedent
    Case = ε "case" Pattern.Patterns Guard ? ':' Block
    Guard = "if" Expr.Named
    SubjectExpr = StarNamedExpr ',' StarNamedExprs ? | Expr.Named

  Import:
    ⊙ = Name | From
    Name = "import" DottedAsName ( ε ',' DottedAsName ) *
    From = ε "from" Dots + "import" Targets | "from" Dots * DottedName "import" Targets
    Dots = '...' | '.'
    Targets = '(' AsName ( ε ',' AsName ) * ',' ? ')' | '*' | AsName ( ε ',' AsName ) *
    AsName = identifier ( "as" identifier ) ?
    DottedAsName = DottedName ( "as" identifier ) ?
    DottedName = identifier ( ε '.' identifier ) *

  Params = Param ( ε ',' Param ) * ',' ?
  Param = '/' | '**' ParamDef | '*' ParamDef ? | ParamDef
  ParamDef = identifier ( ':' not '=' Expr ) ? ( '=' not '=' Expr ) ?

  LambdaParams = LambdaParam ( ε ',' LambdaParam ) * ',' ?
  LambdaParam = '/' | '**' LambdaParamDef | '*' LambdaParamDef ? | LambdaParamDef
  LambdaParamDef = identifier ( '=' not '=' Expr ) ?

  TypeParams = '[' TypeParam ( ε ',' TypeParam ) * ',' ? ']'
  TypeParam = '**' identifier Default ? | '*' identifier Default ? \
            | identifier ( ':' not '=' Expr ) ? Default ?
  Default = '=' not '=' Expr

  Arguments = ( Arg ( ε ',' Arg ) * ',' ? ) ?
  Arg = '**' Expr | '*' Expr | identifier '=' not '=' Expr | Expr.Named ForIfClauses ?

  Slices = Slice ( ε ',' Slice ) * ',' ?
  Slice = Expr ? ':' not '=' Expr ? ( ':' not '=' Expr ? ) ? | '*' Expr | Expr.Named

  StarExprs = StarExpr ( ε ',' StarExpr ) * ',' ?
  StarExpr = '*' Expr.Bitwise | Expr
  StarNamedExprs = StarNamedExpr ( ε ',' StarNamedExpr ) * ',' ?
  StarNamedExpr = '*' Expr.Bitwise | Expr.Named

  ForIfClauses = ForIfClause +
  ForIfClause = "async" ? "for" Target.Stars "in" Expr.Disjunction ( "if" Expr.Disjunction ) *

  Target:
    Stars = Star ( ε ',' Star ) * ',' ?
    Star = '*' Star | Single
    Single = Expr.Primary
    Dels = Single ( ε ',' Single ) * ',' ?

  Expr:
    ⊙ = Lambda | Conditional
    Lambda = "lambda" LambdaParams ? ':' Expr
    Conditional = Disjunction ( "if" Disjunction "else" Expr ) ?
    Disjunction = Conjunction ( ε "or" Conjunction ) *
    Conjunction = Inversion ( ε "and" Inversion ) *
    Inversion = "not" Inversion | Comparison
    Comparison = Bitwise ( CompareOp Bitwise ) *
    CompareOp = [ '==' '!=' '<=' '>=' '<' '>' ] | "not" "in" | "in" | "is" "not" | "is"
    Named = identifier ':=' Expr | Expr
    Yield = "yield" ( "from" Expr | StarExprs ? )

    Bitwise:
      ⊙ = axe Factor
        Term  = ltr  infix '*' '/' '//' '%' '@'
        Sum   = ltr  infix '+' '-'
        Shift = ltr  infix '<<' '>>'
        And   = ltr  infix '&'
        Xor   = ltr  infix '^'
        Or    = ltr  infix '|'

    Factor = [ '+' '-' '~' ] Factor | Power
    Power = Await ( '**' Factor ) ?
    Await = "await" Primary | Primary

    Primary:
      ⊙ = axe Atom
        Trailer = ltr  postfix_nest -> Arguments '(' ')' \
                       postfix_nest -> Slices '[' ']' \
                       infix '.'

      Atom = ( "True" | "False" | "None" | '...' | Strings | number
             | GenExp | Group | Tuple
             | ListComp | List
             | DictComp | SetComp | Dict | Set
             | identifier )
      GenExp = ε '(' Expr.Named ForIfClauses ')'
      Group = ε '(' ( Expr.Yield | Expr.Named ) ')'
      Tuple = '(' ( StarNamedExpr ',' StarNamedExprs ? ) ? ')'
      ListComp = ε '[' Expr.Named ForIfClauses ']'
      List = '[' StarNamedExprs ? ']'
      DictComp = ε '{' KvPair ForIfClauses '}'
      SetComp = ε '{' Expr.Named ForIfClauses '}'
      Dict = ε '{' ( KvPairs ) ? '}'
      Set = '{' StarNamedExprs '}'
      KvPairs = KvPair ( ε ',' KvPair ) * ',' ?
      KvPair = '**' Expr.Bitwise | Expr ':' not '=' Expr

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
