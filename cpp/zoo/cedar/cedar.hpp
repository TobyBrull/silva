#pragma once

#include "canopy/types.hpp"

#include "syntax/seed_interpreter.hpp"

namespace silva::cedar {

  // Translation of https://www.quut.com/c/ANSI-C-grammar-y.html.
  //
  const string_view_t seed_str = R"'(
tokenizer Cedar:
  operator = [ ';' '*' '~' '++' '+=' '+' '--' '-=' '-' '!=' '!' '...' ]
  include tokenizer FreeForm

language Cedar:
  ⊙ = Decl *
  Decl:
    ⊙ = Decl.Specifiers Type.Specifier InitDeclarator ( ';' | Stmt.Compound ) \
      | StaticAssert
    Specifiers = ( StorageClassSpecifier | FunctionSpecifier | AlignmentSpecifier | Type.Qualifier ) *
    StorageClassSpecifier = ( 'typedef' | 'extern' | 'static' | '_Thread_local' | 'auto' | 'register' )
    FunctionSpecifier = 'inline' | '_Noreturn'
    AlignmentSpecifier = ( '_Alignas' | 'alignas' ) '(' ( Type.Name | Expr.Const ) ')'
    StaticAssert = '_Static_assert' '(' Expr.Const ',' string ')' ';'

    Struct = Type.SpecifierQualifierList StructDeclaratorList ? | Decl.StaticAssert
    StructDeclaratorList = StructDeclarator ( ',' StructDeclarator ) *
    StructDeclarator = Declarator ( ':' Expr.Const ) ? | ':' Expr.Const
    InitDeclarator = Declarator ( '=' Initializer ) ?
    Declarator = Pointer ? DirectDeclarator
    AbstractDeclarator = Pointer DirectAbstractDeclarator ? | DirectAbstractDeclarator
    DirectAbstractDeclarator = ( '(' AbstractDeclarator ')' | DirectAbstractDeclaratorExt ) DirectAbstractDeclaratorExt *
    DirectAbstractDeclaratorExt = CommonDeclaratorExt
    Pointer = '*' Type.Qualifier * Pointer ?
    DirectDeclarator = ( identifier | '(' Declarator ')' ) DirectDeclaratorExt *
    DirectDeclaratorExt = ( CommonDeclaratorExt
                          | '(' IdentifierList ')'
                          )
    CommonDeclaratorExt = alias ( '[' ']'
                                | '(' ParameterList ? ')'
                                )
    ParameterList = ParameterDeclaration ( ',' ParameterDeclaration ) *
    ParameterDeclaration = Decl.Specifiers Type.Specifier Type.Name ( Declarator | AbstractDeclarator ) ? | '...'
    IdentifierList = identifier ( ',' identifier ) *

    Initializer = '{' InitializerList ',' ? '}' | Expr
    InitializerList = Designation ? Initializer ( ',' Designation ? Initializer ) *
    Designation = Designator + '='
    Designator = '.' identifier | '[' Expr.Const ']'

  Type:
    Specifier = ( 'void' | 'char' | 'short' | 'int' | 'long'
                | 'float' | 'double' | 'signed' | 'unsigned'
                | '_Bool' | '_Complex' | '_Imaginary'
                | AtomicTypeSpecifier
                | StructOrUnionSpecifier
                | EnumSpecifier
                | TypedefName
                )
    TypedefName = identifier # TODO: should only match identifiers that have previously been typedef'ed.
    Qualifier = ( 'const' | 'volatile' | 'restrict' | '_Atomic' )
    SpecifierQualifierList = ( Type.Specifier | Type.Qualifier ) *

    EnumSpecifier = 'enum' identifier ? '{' Enumerator ( ',' Enumerator ) * ',' ? '}'
    Enumerator = EnumerationConstant ( '=' Expr.Const ) ?
    Name = SpecifierQualifierList Decl.AbstractDeclarator ?

    EnumerationConstant = identifier

    AtomicTypeSpecifier = '_Atomic' '(' Name ')'
    StructOrUnionSpecifier = ( 'struct' | 'union' ) identifier ? ( '{' Decl.Struct * '}' ) ?

  Stmt:
    ⊙ = ( Label | Case | Default
        | Compound
        | ExprStmt
        | If | Switch
        | While | DoWhile | For
        | Goto | Continue | Break | Return
        )
    Label = identifier ':' Stmt
    Case = 'case' Expr.Const ':' Stmt
    Default = 'default' ':' Stmt
    Compound = '{' ( Stmt | Decl ) * '}'
    ExprStmt = Expr ? ';'
    If = 'if' '(' Expr ')' Stmt ( 'else' Stmt ) ?
    Switch = 'switch' '(' Expr ')' Stmt
    While = 'while' '(' Expr ')' Stmt
    DoWhile = 'do' Stmt 'while' '(' Expr ')' ';'
    For = 'for' '(' ( Decl | ExprStmt ) Expr ? ';' Expr ? ')' Stmt
    Goto = 'goto' identifier ';'
    Continue = 'continue' ';'
    Break = 'break' ';'
    Return = 'return' Expr ? ';'

  Expr:
    ⊙ = axe Atom
      Parens  = nest atom_nest '(' ')'
      Postfix = ltr  postfix '++' '--' \
                     postfix_nest -> ExprOrNone '(' ')' '[' ']' \
                     infix '.' '->'
      Prefix  = rtl  prefix '++' '--' '+' '-' '!' '~' '*' '&' 'sizeof' '_Alignof'
      Mult    = ltr  infix '*' '/' '%'
      Add     = ltr  infix '+' '-'
      Shift   = ltr  infix '<<' '>>'
      Rel     = ltr  infix '<' '>' '<=' '>='
      Eq      = ltr  infix '==' '!='
      BitAnd  = ltr  infix '&'
      BitXor  = ltr  infix '^'
      BitOr   = ltr  infix '|'
      LogAnd  = ltr  infix '&&'
      LogOr   = ltr  infix '||'
      Tern    = rtl  ternary '?' ':'
      Assign  = rtl  infix '=' '+=' '-=' '*=' '/=' '%=' '<<=' '>>=' '&=' '^=' '|='
      Comma   = ltr  infix_flat ','
    Atom = number | string + | identifier
    ExprOrNone = Expr | None
    Const = Expr # not allowed to be Expr.Comma or Expr.Assign
)'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t);
}
