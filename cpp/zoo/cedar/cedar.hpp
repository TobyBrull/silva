#pragma once

#include "canopy/types.hpp"

#include "syntax/seed_interpreter.hpp"

namespace silva::cedar {

  // Adoption of a subset of Annex A from the C standard, e.g.,
  // https://port70.net/~nsz/c/c11/n1570.html#A
  //
  const string_view_t seed_str = R"'(
language Cedar:
  skip = skip.free_form

  operator = [ '...' '.' ';' ':' '*=' '*' '/=' '/' '%=' '%' '~'
               '++' '+=' '+' '--' '-=' '->' '-'
               '!=' '!' '==' '=' '<<=' '<<' '<=' '<' '>>=' '>>' '>=' '>'
               '&&' '&=' '&' '||' '|=' '|' '^=' '^' '?' ',' ]

  ⊙ = Declaration *

  Declaration:
    ⊙ = Specifiers Init ? ( ';' | Stmt.Compound )
    Specifiers = [ StorageClassSpecifier FunctionSpecifier AlignmentSpecifier Type.Qualifier Type.Specifier ] +
    StorageClassSpecifier = [ "typedef" "extern" "static" "_Thread_local" ]
    FunctionSpecifier = "inline"
    AlignmentSpecifier = "alignas" '(' ( Type.Name | Expr.Conditional ) ')'

    ParameterList = ParameterDeclaration ( ',' ParameterDeclaration ) *
    ParameterDeclaration = Specifiers ( Type.Declarator | Type.Declarator.Abstract ) ? | '...'

    Init = Type.Declarator ( '=' Initializer ) ?
    Initializer = '{' InitializerList ',' ? '}' | Expr.Assignment
    InitializerList = Initializer ( ε ',' Initializer ) *

  Type:
    IntSpecifier = [ "signed" "unsigned" "short" "long" "char" "int" ] +
    FloatSpecifier = [ "float" "double" ]
    Specifier = [ "void" IntSpecifier FloatSpecifier AtomicTypeSpecifier StructSpecifier EnumSpecifier TypedefName ]

    # TODO: should only match identifiers that have previously been typedef'ed.
    TypedefName = not ε

    Qualifier = [ "const" "volatile" "restrict" "_Atomic" ]
    SpecifierQualifierList = ( Qualifier | Specifier ) +
    Name = SpecifierQualifierList Declarator.Abstract ?
    AtomicTypeSpecifier = "_Atomic" '(' Name ')'
    StructSpecifier = ( "struct" | "union" ) identifier ? ( '{' StructMemberSpecifier * '}' ) ?
    StructMemberSpecifier = SpecifierQualifierList Declarator ? ';'

    EnumSpecifier = "enum" identifier ? '{' Enumerator ( ',' Enumerator ) * ',' ? '}'
    Enumerator = identifier ( '=' Expr.Conditional ) ?

    Declarator:
      ⊙ = Pointer ? Direct
      Direct = ( identifier | '(' Declarator ')' ) Suffix *
      Abstract = Pointer DirectAbstract ? | DirectAbstract
      DirectAbstract = ( ε '(' Abstract ')' ) ? Suffix *
      Pointer = '*' Type.Qualifier * Pointer ?
      Suffix = '(' Declaration.ParameterList ? ')' | '[' Expr.Conditional ? ']'

  Stmt:
    ⊙ = [ Compound If Switch Case Default While DoWhile For Continue Break Return Goto Label ExprStmt ]
    Compound = '{' ( Stmt | Declaration ) * '}'
    If = "if" '(' Expr ')' Stmt ( "else" Stmt ) ?
    Switch = "switch" '(' Expr ')' Stmt
    Case = "case" Expr.Conditional ':' Stmt
    Default = "default" ':' Stmt
    While = "while" '(' Expr ')' Stmt
    DoWhile = "do" Stmt "while" '(' Expr ')' ';'
    For = "for" '(' ( Declaration | ExprStmt ) Expr ? ';' Expr ? ')' Stmt
    Continue = "continue" ';'
    Break = "break" ';'
    Return = "return" Expr ? ';'
    Label = identifier ':' Stmt
    Goto = "goto" identifier ';'
    ExprStmt = Expr ? ';'

  Expr:
    ⊙ = axe Atom oper
      Postfix     = ltr  postfix '++' '--' \
                         postfix_nest -> ExprOrNone '(' ')' '[' ']' \
                         infix '.' '->'
      Unary       = rtl  prefix '++' '--' '+' '-' '!' '~' '*' '&' \
                         prefix_nest -> Type.Name '(' ')'
      Mult        = ltr  infix '*' '/' '%'
      Add         = ltr  infix '+' '-'
      Shift       = ltr  infix '<<' '>>'
      Relational  = ltr  infix '<' '>' '<=' '>='
      Equality    = ltr  infix '==' '!='
      BitwiseAnd  = ltr  infix '&'
      BitwiseXor  = ltr  infix '^'
      BitwiseOr   = ltr  infix '|'
      LogicalAnd  = ltr  infix '&&'
      LogicalOr   = ltr  infix '||'
      Conditional = rtl  ternary '?' ':'
      Assignment  = rtl  infix '=' '+=' '-=' '*=' '/=' '%=' '<<=' '>>=' '&=' '^=' '|='
      Comma       = ltr  infix_flat ','
    Atom = number | string + | identifier | '(' Expr ')' | Sizeof | Alignof
    oper = operator | parenthesis
    Sizeof = "sizeof" ( Expr.Unary | '(' Type.Name ')' )
    Alignof = "_Alignof" '(' Type.Name ')'
    ExprOrNone = Expr | None
)'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t);
}
