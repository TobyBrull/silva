#pragma once

#include "canopy/types.hpp"

#include "syntax/seed_interpreter.hpp"

namespace silva::cedar {

  // Translation of https://www.quut.com/c/ANSI-C-grammar-y.html.
  //
  const string_view_t seed_str = R"'(
tokenizer Cedar:
  operator = [ '...' '.' ';' ':' '*=' '*' '/=' '/' '%=' '%' '~' '++' '+=' '+' '--' '-=' '->' '-'
               '!=' '!' '==' '=' '<<=' '<<' '<=' '<' '>>=' '>>' '>=' '>'
               '&&' '&=' '&' '||' '|=' '|' '^=' '^' '?' ',' ]
  keyword = [ 'auto' 'break' 'case' 'char' 'const' 'continue' 'default' 'do'
              'double' 'else' 'enum' 'extern' 'float' 'for' 'goto' 'if'
              'inline' 'int' 'long' 'register' 'restrict' 'return' 'short'
              'signed' 'sizeof' 'static' 'struct' 'switch' 'typedef' 'union'
              'unsigned' 'void' 'volatile' 'while'
              '_Alignas' '_Alignof' '_Atomic' '_Bool' '_Complex'
              '_Imaginary' '_Noreturn' '_Static_assert' '_Thread_local'
              'alignas' ]
  include tokenizer FreeForm

language Cedar:
  ⊙ = Declaration *
  Declaration:
    ⊙ = Specifiers Init ? ( ';' | Stmt.Compound ) \
      | StaticAssert
    Specifiers = ( StorageClassSpecifier | FunctionSpecifier | AlignmentSpecifier | Type.Qualifier ) * Type.Specifier
    StorageClassSpecifier = ( 'typedef' | 'extern' | 'static' | '_Thread_local' | 'auto' | 'register' )
    FunctionSpecifier = 'inline' | '_Noreturn'
    AlignmentSpecifier = ( '_Alignas' | 'alignas' ) '(' ( Type.Name | Expr.Conditional ) ')'
    StaticAssert = '_Static_assert' '(' Expr.Conditional ',' string ')' ';'

    ParameterList = ParameterDeclaration ( ',' ParameterDeclaration ) *
    ParameterDeclaration = Specifiers ( Declarator | Declarator.Abstract ) ? | '...'
    IdentifierList = identifier ( ',' identifier ) *

    Designation = Designator + '='
    Designator = '.' identifier | '[' Expr.Conditional ']'

    Init = Declarator ( '=' Initializer ) ?
    Initializer = '{' InitializerList ',' ? '}' | Expr.Assignment
    InitializerList = Designation ? Initializer ( ',' Designation ? Initializer ) *

  Declarator:
    ⊙ = Pointer ? Direct
    Abstract = Pointer DirectAbstract ? | DirectAbstract
    DirectAbstract = ( '(' Abstract ')' | DirectAbstractExt ) DirectAbstractExt *
    DirectAbstractExt = CommonExt
    Pointer = '*' Type.Qualifier * Pointer ?
    Direct = ( identifier | '(' Declarator ')' ) DirectExt *
    DirectExt = ( CommonExt | '(' Declaration.IdentifierList ')' )
    CommonExt = alias ( '[' Type.Qualifier * Expr ? ']'
                      | '(' Declaration.ParameterList ? ')'
                      )


  Type:
    IntSpecifier = ( 'signed' | 'unsigned' | 'short' | 'long' | 'char' | 'int' ) +
    BuiltinSpecifier = 'float' | 'double' | '_Bool' | '_Complex' | '_Imaginary'
    Specifier = ( 'void'
                | IntSpecifier
                | BuiltinSpecifier
                | AtomicTypeSpecifier
                | StructOrUnionSpecifier
                | EnumSpecifier
#                | TypedefName
                )
    TypedefName = identifier # TODO: should only match identifiers that have previously been typedef'ed.
    Qualifier = ( 'const' | 'volatile' | 'restrict' | '_Atomic' )
    SpecifierQualifierList = Type.Qualifier * Type.Specifier

    EnumSpecifier = 'enum' identifier ? '{' Enumerator ( ',' Enumerator ) * ',' ? '}'
    Enumerator = EnumerationConstant ( '=' Expr.Conditional ) ?
    Name = SpecifierQualifierList Declarator.Abstract ?

    EnumerationConstant = identifier

    AtomicTypeSpecifier = '_Atomic' '(' Name ')'
    StructOrUnionSpecifier = ( 'struct' | 'union' ) identifier ? ( '{' StructMemberDecl * '}' ) ?
    StructMemberDecl = Type.SpecifierQualifierList StructMemberDeclaratorList ? ';' | Declaration.StaticAssert
    StructMemberDeclaratorList = StructMemberDeclarator ( ',' StructMemberDeclarator ) *
    StructMemberDeclarator = Declarator ( ':' Expr.Conditional ) ? | ':' Expr.Conditional

  Stmt:
    ⊙ = ( Compound
        | If | Switch | Case | Default
        | While | DoWhile | For
        | Continue | Break | Return
        | Goto | Label
        | ExprStmt
        )
    Compound = '{' ( Stmt | Declaration ) * '}'
    If = 'if' '(' Expr ')' Stmt ( 'else' Stmt ) ?
    Switch = 'switch' '(' Expr ')' Stmt
    Case = 'case' Expr.Conditional ':' Stmt
    Default = 'default' ':' Stmt
    While = 'while' '(' Expr ')' Stmt
    DoWhile = 'do' Stmt 'while' '(' Expr ')' ';'
    For = 'for' '(' ( Declaration | ExprStmt ) Expr ? ';' Expr ? ')' Stmt
    Continue = 'continue' ';'
    Break = 'break' ';'
    Return = 'return' Expr ? ';'
    Label = identifier ':' Stmt
    Goto = 'goto' identifier ';'
    ExprStmt = Expr ? ';'

  Expr:
    ⊙ = axe Atom
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
    Sizeof = 'sizeof' ( Expr.Unary | '(' Type.Name ')' )
    Alignof = '_Alignof' '(' Type.Name ')'
    ExprOrNone = Expr | None
)'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t);
}
